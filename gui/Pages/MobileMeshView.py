# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2011 EDF S.A.
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301, USA.

#-------------------------------------------------------------------------------

"""
This module defines the values of reference.

This module contains the following classes and function:
- MobileMeshView
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Toolbox   import GuiParam
from Pages.MobileMeshForm  import Ui_MobileMeshForm
from Base.QtPage    import setGreenColor, IntValidator,  ComboModel
from Pages.MobileMeshModel import MobileMeshModel

from Pages.QMeiEditorView import QMeiEditorView

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("MobileMeshView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class MobileMeshView(QWidget, Ui_MobileMeshForm):
    """
    Class to open Page.
    """
    viscosity_iso = """# Viscosity of the mesh allows to control the deformation
# of the mesh. Viscosity must be greater than zero.
# It could be isotropic (the same for all directions) or
# orthotropic.
#
# In the following example, a hight value of viscosity
# is imposed around a mobile cylinder.
# The viscosity is specfied for all cells
# on the initial mesh before any deformation.
#
xr2 = 1.5^2;
xcen = 5.0;
ycen = 0.;
zcen = 6.0;
xray2 = (x-xcen)^2 + (y-ycen)^2 + (z-zcen)^2;
mesh_vi1 = 1;
if (xray2 < xr2) mesh_vi1 = 1e10;
"""

    viscosity_ortho = """# Viscosity of the mesh allows to control the deformation
# of the mesh. Viscosity must be greater than zero.
# It could be isotropic (the same for all directions) or
# orthotropic.
#
# In the following example, a hight value of viscosity
# is imposed around a mobile cylinder.
# The viscosity is specfied for all cells
# on the initial mesh before any deformation.
#
xr2 = 1.5^2;
xcen = 5.0;
ycen = 0.;
zcen = 6.0;
xray2 = (x-xcen)^2 + (y-ycen)^2 + (z-zcen)^2;
mesh_vi1 = 1;
mesh_vi2 = 1;
mesh_vi3 = 1;
if (xray2 < xr2) {
    mesh_vi1 = 1e10;
    mesh_vi2 = 1e10;
    mesh_vi3 = 1e10;
}
"""

    def __init__(self, parent, case, browser):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_MobileMeshForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.mdl = MobileMeshModel(self.case)
        self.browser = browser

        # Combo model VISCOSITY
        self.modelVISCOSITY = ComboModel(self.comboBoxVISCOSITY,2,1)

        self.modelVISCOSITY.addItem(self.tr("isotropic"), 'isotrop')
        self.modelVISCOSITY.addItem(self.tr("orthotropic"), 'orthotrop')

        # Combo model MEI
        self.modelMEI = ComboModel(self.comboBoxMEI, 2, 1)

        self.modelMEI.addItem(self.tr("user subroutine (usvima)"), 'user_subroutine')
        self.modelMEI.addItem(self.tr("user formula"), 'user_function')

        # Connections
        self.connect(self.groupBoxALE, SIGNAL("clicked(bool)"), self.slotMethod)
        self.connect(self.lineEditNALINF, SIGNAL("textChanged(const QString &)"), self.slotNalinf)
        self.connect(self.comboBoxVISCOSITY, SIGNAL("activated(const QString&)"), self.slotViscosityType)
        self.connect(self.comboBoxMEI, SIGNAL("activated(const QString&)"), self.slotMEI)
        self.connect(self.pushButtonFormula, SIGNAL("clicked(bool)"), self.slotFormula)

        # Validators
        validatorNALINF = IntValidator(self.lineEditNALINF, min=0)
        self.lineEditNALINF.setValidator(validatorNALINF)

        if self.mdl.getMethod() == 'on':
            self.groupBoxALE.setChecked(True)
            checked = True
        else:
            self.groupBoxALE.setChecked(False)
            checked = False

        self.slotMethod(checked)

        # Enable / disable formula state
        self.slotMEI(self.comboBoxMEI.currentText())
        setGreenColor(self.pushButtonFormula, False)


    @pyqtSignature("bool")
    def slotMethod(self, checked):
        """
        Private slot.

        Activates ALE method.

        @type checked: C{True} or C{False}
        @param checked: if C{True}, shows the QGroupBox ALE parameters
        """
        self.groupBoxALE.setFlat(not checked)
        if checked:
            self.frame.show()
            self.mdl.setMethod ("on")
            nalinf = self.mdl.getSubIterations()
            self.lineEditNALINF.setText(QString(str(nalinf)))
            value = self.mdl.getViscosity()
            self.modelVISCOSITY.setItem(str_model=value)
            value = self.mdl.getMEI()
            self.modelMEI.setItem(str_model=value)
        else:
            self.frame.hide()
            self.mdl.setMethod("off")
        self.browser.configureTree(self.case)


    @pyqtSignature("const QString&")
    def slotNalinf(self, text):
        """
        Input viscosity type of mesh : isotrop or orthotrop.
        """
        nalinf, ok = text.toInt()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setSubIterations(nalinf)


    @pyqtSignature("const QString&")
    def slotViscosityType(self, text):
        """
        Input viscosity type of mesh : isotrop or orthotrop.
        """
        self.viscosity_type = self.modelVISCOSITY.dicoV2M[str(text)]
        visco = self.viscosity_type
        self.mdl.setViscosity(visco)
        return visco


    @pyqtSignature("const QString&")
    def slotMEI(self, text):
        """
        MEI
        """
        MEI = self.modelMEI.dicoV2M[str(text)]
        self.MEI = MEI
        self.mdl.setMEI(MEI)
        # enable disable formula button

        isFormulaButtonEnabled = MEI == 'user_function'
        self.pushButtonFormula.setEnabled(isFormulaButtonEnabled)
        setGreenColor(self.pushButtonFormula, isFormulaButtonEnabled)

        return MEI


    @pyqtSignature("const QString&")
    def slotFormula(self, text):
        """
        Run formula editor.
        """
        exp = self.mdl.getFormula()

        if self.mdl.getViscosity() == 'isotrop':
            if not exp:
                exp = "mesh_vi1 = 1;"
            req = [('mesh_vi1', 'mesh viscosity')]
            exa = MobileMeshView.viscosity_iso
        else:
            if not exp:
                exp = "mesh_vi1 = 1;\nmesh_vi2 = 1;\nmesh_vi3 = 1;"
            req = [('mesh_vi1', 'mesh viscosity X'),
                   ('mesh_vi2', 'mesh viscosity Y'),
                   ('mesh_vi3', 'mesh viscosity Z')]
            exa = MobileMeshView.viscosity_ortho

        symb = [('x', "X cell's gravity center"),
                ('y', "Y cell's gravity center"),
                ('z', "Z cell's gravity center"),
                ('dt', 'time step'),
                ('t', 'current time'),
                ('iter', 'number of iteration')]

        dialog = QMeiEditorView(self,expression = exp,
                                     required   = req,
                                     symbols    = symb,
                                     examples   = exa)
        if dialog.exec_():
            result = dialog.get_result()
            log.debug("slotFormulaMobileMeshView -> %s" % str(result))
            self.mdl.setFormula(result)
            setGreenColor(self.pushButtonFormula, False)


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
