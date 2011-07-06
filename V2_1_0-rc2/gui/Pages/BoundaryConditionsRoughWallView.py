# -*- coding: utf-8 -*-
#
#-------------------------------------------------------------------------------
#
#     This file is part of the Code_Saturne User Interface, element of the
#     Code_Saturne CFD tool.
#
#     Copyright (C) 1998-2009 EDF S.A., France
#
#     contact: saturne-support@edf.fr
#
#     The Code_Saturne User Interface is free software; you can redistribute it
#     and/or modify it under the terms of the GNU General Public License
#     as published by the Free Software Foundation; either version 2 of
#     the License, or (at your option) any later version.
#
#     The Code_Saturne User Interface is distributed in the hope that it will be
#     useful, but WITHOUT ANY WARRANTY; without even the implied warranty
#     of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
#
#     You should have received a copy of the GNU General Public License
#     along with the Code_Saturne Kernel; if not, write to the
#     Free Software Foundation, Inc.,
#     51 Franklin St, Fifth Floor,
#     Boston, MA  02110-1301  USA
#
#-------------------------------------------------------------------------------

"""
This module contains the following classes:
- BoundaryConditionsRoughWallView
"""

#-------------------------------------------------------------------------------
# Standard modules
#-------------------------------------------------------------------------------

import string, logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Pages.BoundaryConditionsRoughWallForm import Ui_BoundaryConditionsRoughWallForm

from Base.Toolbox import GuiParam
from Base.QtPage import DoubleValidator, ComboModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("BoundaryConditionsRoughWallView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class BoundaryConditionsRoughWallView(QWidget, Ui_BoundaryConditionsRoughWallForm):
    """
    Boundary condition for smooth or rough wall.
    """
    def __init__(self, parent):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_BoundaryConditionsRoughWallForm.__init__(self)
        self.setupUi(self)


    def setup(self, case):
        """
        Setup the widget
        """
        self.__case = case
        self.__boundary = None

        self.connect(self.radioButtonSmooth, SIGNAL("clicked()"), self.__slotRoughness)
        self.connect(self.radioButtonRough,  SIGNAL("clicked()"), self.__slotRoughness)

        self.connect(self.lineEditRoughCoef, SIGNAL("textChanged(const QString &)"), self.__slotRoughnessHeight)

        validatorRoughCoef = DoubleValidator(self.lineEditRoughCoef)
        self.lineEditRoughCoef.setValidator(validatorRoughCoef)


    def showWidget(self, boundary):
        """
        Show the widget
        """
        self.show()
        self.__boundary = boundary

        if self.__boundary.getRoughnessChoice() == "on":
            self.radioButtonSmooth.setChecked(False)
            self.radioButtonRough.setChecked(True)
        else:
            self.radioButtonSmooth.setChecked(True)
            self.radioButtonRough.setChecked(False)

        self.__slotRoughness()


    def hideWidget(self):
        """
        Hide the widget
        """
        self.hide()


    @pyqtSignature("")
    def __slotRoughness(self):
        """
        Private slot.

        Selects if the wall is rought or smooth.
        """
        if self.radioButtonSmooth.isChecked():
            self.frameRoughness.hide()
            self.__boundary.setRoughnessChoice('off')

        elif self.radioButtonRough.isChecked():
            self.frameRoughness.show()
            self.__boundary.setRoughnessChoice('on')
            r = self.__boundary.getRoughness()
            self.lineEditRoughCoef.setText(QString(str(r)))


    @pyqtSignature("const QString&")
    def __slotRoughnessHeight(self, text):
        """
        Private slot.

        Input the roughness height for the selected wall.

        @type text: C{QString}
        @param text: roughness height.
        """
        r, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setRoughness(r)


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
