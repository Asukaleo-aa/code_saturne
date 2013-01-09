# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2013 EDF S.A.
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
This module contains the following classes:
- ElectricalView
"""

#-------------------------------------------------------------------------------
# Standard modules
#-------------------------------------------------------------------------------

import os, logging

#-------------------------------------------------------------------------------
# Third-party modules
#-------------------------------------------------------------------------------

from PyQt4.QtCore import *
from PyQt4.QtGui  import *

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from ElectricalForm import Ui_ElectricalForm

from Base.Toolbox import GuiParam
from Base.Common import LABEL_LENGTH_MAX
from Base.QtPage import ComboModel, DoubleValidator, RegExpValidator, setGreenColor

from Pages.ElectricalModel import ElectricalModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("ElectricalView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class ElectricalView(QWidget, Ui_ElectricalForm):
    """
    """
    def __init__(self, parent, case, stbar):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_ElectricalForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.stbar = stbar
        self.case.undoStopGlobal()

        self.model = ElectricalModel(self.case)

        # Combo model
        self.modelJoule = ComboModel(self.comboBoxJouleModel, 4, 1)
        self.modelJoule.addItem(self.tr("AC/DC"), "AC/DC")
        self.modelJoule.addItem(self.tr("three-phase"), "three-phase")
        self.modelJoule.addItem(self.tr("AC/DC with Transformer coupling"), "AC/DC+Transformer")
        self.modelJoule.addItem(self.tr("three-phase with Transformer coupling"), "three-phase+Transformer")
        self.modelJoule.disableItem(str_model="AC/DC+Transformer")
        self.modelJoule.disableItem(str_model="three-phase+Transformer")

        self.modelScaling = ComboModel(self.comboBoxScalingModel, 3, 1)
        self.modelScaling.addItem(self.tr("general case"), "general_case")
        self.modelScaling.addItem(self.tr("plane define"), "plane_define")
        self.modelScaling.addItem(self.tr("user define"), "user")

        self.modelDirection = ComboModel(self.comboBoxDirection, 3, 1)
        self.modelDirection.addItem(self.tr("X"), "X")
        self.modelDirection.addItem(self.tr("Y"), "Y")
        self.modelDirection.addItem(self.tr("Z"), "Z")

        # Connections
        self.connect(self.pushButtonPropertiesData, SIGNAL("pressed()"), self.__slotSearchPropertiesData)
        self.connect(self.lineEditSRROM,            SIGNAL("textChanged(const QString &)"), self.slotSRROM)
        self.connect(self.lineEditPower,            SIGNAL("textChanged(const QString &)"), self.slotPower)
        self.connect(self.lineEditCurrent,          SIGNAL("textChanged(const QString &)"), self.slotCurrent)
        self.connect(self.checkBoxScaling,          SIGNAL("clicked()"), self.slotScaling)
        self.connect(self.comboBoxJouleModel,       SIGNAL("activated(const QString&)"), self.slotJouleModel)
        self.connect(self.comboBoxScalingModel,     SIGNAL("activated(const QString&)"), self.slotScalingModel)
        self.connect(self.comboBoxDirection,        SIGNAL("clicked()"), self.slotDirection)
        self.connect(self.lineEditPlaneDefinition,  SIGNAL("textChanged(const QString &)"), self.slotPlaneDef)

        # Validators
        validatorSRROM = DoubleValidator(self.lineEditSRROM, min=0.0, max=1.0)
        validatorSRROM.setExclusiveMin(False)
        validatorPower = DoubleValidator(self.lineEditPower, min=0.0)
        validatorPower.setExclusiveMin(False)
        validatorCurrent = DoubleValidator(self.lineEditCurrent, min=0.0)
        validatorCurrent.setExclusiveMin(False)
        self.lineEditSRROM.setValidator(validatorSRROM)
        self.lineEditPower.setValidator(validatorPower)
        self.lineEditCurrent.setValidator(validatorCurrent)

        # Initialize widget
        self.__initializeWidget()

        self.case.undoStartGlobal()


    @pyqtSignature("")
    def __initializeWidget(self):
        """
        Initialize widget
        """
        name = self.model.getPropertiesDataFileName()
        if name != None:
            self.labelPropertiesFile.setText(QString(name))
            setGreenColor(self.pushButtonPropertiesData, False)
        else:
            setGreenColor(self.pushButtonPropertiesData, True)

        srrom = self.model.getSRROM()
        self.lineEditSRROM.setText(QString(str(srrom)))

        self.groupBoxRecalage.hide()

        if self.model.getScaling() == 'on':
            self.checkBoxScaling.setChecked(True)
            self.labelScalingModel.show()
            self.comboBoxScalingModel.show()
        else:
            self.checkBoxScaling.setChecked(False)
            self.labelScalingModel.hide()
            self.comboBoxScalingModel.hide()

        if self.model.getElectricalModel() == "joule":
            self.groupBoxJoule.show()
            self.groupBoxElectricArc.hide()

            model = self.model.getJouleModel()
            self.modelJoule.setItem(str_model=str(model))
            power = self.model.getPower()
            self.lineEditPower.setText(QString(str(power)))

            self.labelPropertiesData.hide()
            self.pushButtonPropertiesData.hide()
            self.labelPropertiesFile.hide()

            self.pushButtonPropertiesData.hide()
            self.labelPropertiesData.hide()
            self.labelPropertiesFile.hide()

        elif self.model.getElectricalModel() == "arc":
            self.groupBoxJoule.hide()
            self.groupBoxElectricArc.show()

            current = self.model.getCurrent()
            self.lineEditCurrent.setText(QString(str(current)))

            if self.model.getScaling() == 'on':
                model = self.model.getScalingModel()
                self.modelScaling.setItem(str_model=str(model))
                if model == 'plane_define':
                    self.groupBoxRecalage.show()
                    direction = self.model.getDirection()
                    self.modelDirection.setItem(str_model=str(direction))
                    definition = self.model.getPlaneDefinition()
                    self.lineEditPlaneDefinition.setText(QString(str(definition)))


    @pyqtSignature("")
    def __slotSearchPropertiesData(self):
        """
        Select a properties file of data for electric arc
        """
        data = self.case['data_path']
        title = self.tr("Properties file of data.")
        filetypes = self.tr("Properties data (*dp_ELE*);;All Files (*)")
        file = QFileDialog.getOpenFileName(self, title, data, filetypes)
        file = str(file)
        if not file:
            return
        file = os.path.basename(file)
        if file not in os.listdir(data):
            title = self.tr("WARNING")
            msg   = self.tr("This selected file is not in the DATA directory")
            QMessageBox.information(self, title, msg)
        else:
            self.labelPropertiesFile.setText(QString(file))
            self.model.setPropertiesDataFileName(file)
            setGreenColor(self.pushButtonPropertiesData, False)


    @pyqtSignature("const QString &")
    def slotSRROM(self, text):
        """
        Input Relaxation coefficient for mass density
        """
        srrom, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setSRROM(srrom)


    @pyqtSignature("const QString &")
    def slotPower(self, text):
        """
        Input Imposed Power
        """
        power, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setPower(power)


    @pyqtSignature("const QString &")
    def slotCurrent(self, text):
        """
        Input Imposed current intensity
        """
        current, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.model.setCurrent(current)


    @pyqtSignature("")
    def slotJouleModel(self, text):
        """
        Input Joule model.
        """
        model = self.modelJoule.dicoV2M[str(text)]
        self.model.setJouleModel(model)


    @pyqtSignature("")
    def slotScaling(self):
        """
        Input "Electric variables" scaling.
        """
        if self.checkBoxScaling.isChecked():
            self.model.setScaling("on")
        else:
            self.model.setScaling("off")

        self.__initializeWidget()


    @pyqtSignature("")
    def slotScalingModel(self, text):
        """
        Input scaling model.
        """
        model = self.modelScaling.dicoV2M[str(text)]
        self.model.setScalingModel(model)
        self.__initializeWidget()


    @pyqtSignature("")
    def slotDirection(self, text):
        """
        Input current density direction for scaling.
        """
        direction = self.modelDirection.dicoV2M[str(text)]
        self.model.setDirection(direction)


    @pyqtSignature("const QString &")
    def slotPlaneDef(self, text):
        """
        Input Imposed Power
        """
        self.model.setPlaneDefinition(text)


    def tr(self, text):
        """
        Translation.
        """
        return text


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
