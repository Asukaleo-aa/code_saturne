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
This module defines the values of reference.

This module contains the following classes and function:
- ReferenceValuesView
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

from Base.Toolbox import GuiParam
from Pages.ReferenceValuesForm import Ui_ReferenceValuesForm
import Base.QtPage as QtPage
from Pages.ReferenceValuesModel import ReferenceValuesModel
from Pages.GasCombustionModel import GasCombustionModel
from Pages.CompressibleModel import CompressibleModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("ReferenceValuesView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class ReferenceValuesView(QWidget, Ui_ReferenceValuesForm):
    """
    Class to open Reference Pressure Page.
    """
    def __init__(self, parent, case):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_ReferenceValuesForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.case.undoStopGlobal()
        self.mdl = ReferenceValuesModel(self.case)

        # Combo models
        self.modelLength = QtPage.ComboModel(self.comboBoxLength,2,1)
        self.modelLength.addItem(self.tr("Automatic"), 'automatic')
        self.modelLength.addItem(self.tr("Prescribed"), 'prescribed')
        self.comboBoxLength.setSizeAdjustPolicy(QComboBox.AdjustToContents)

        # Connections

        self.connect(self.lineEditP0,        SIGNAL("textChanged(const QString &)"), self.slotPressure)
        self.connect(self.lineEditV0,        SIGNAL("textChanged(const QString &)"), self.slotVelocity)
        self.connect(self.comboBoxLength,    SIGNAL("activated(const QString&)"),    self.slotLengthChoice)
        self.connect(self.lineEditL0,        SIGNAL("textChanged(const QString &)"), self.slotLength)
        self.connect(self.lineEditT0,        SIGNAL("textChanged(const QString &)"), self.slotTemperature)
        self.connect(self.lineEditOxydant,   SIGNAL("textChanged(const QString &)"), self.slotTempOxydant)
        self.connect(self.lineEditFuel,      SIGNAL("textChanged(const QString &)"), self.slotTempFuel)
        self.connect(self.lineEditMassMolar, SIGNAL("textChanged(const QString &)"), self.slotMassemol)

        # Validators

        validatorP0 = QtPage.DoubleValidator(self.lineEditP0, min=0.0)
        self.lineEditP0.setValidator(validatorP0)

        validatorV0 = QtPage.DoubleValidator(self.lineEditV0, min=0.0)
        self.lineEditV0.setValidator(validatorV0)

        validatorL0 = QtPage.DoubleValidator(self.lineEditL0, min=0.0)
        self.lineEditL0.setValidator(validatorL0)

        validatorT0 = QtPage.DoubleValidator(self.lineEditT0,  min=0.0)
        validatorT0.setExclusiveMin(True)
        self.lineEditT0.setValidator(validatorT0)

        validatorOxydant = QtPage.DoubleValidator(self.lineEditOxydant,  min=0.0)
        validatorOxydant.setExclusiveMin(True)
        self.lineEditOxydant.setValidator(validatorOxydant)

        validatorFuel = QtPage.DoubleValidator(self.lineEditFuel,  min=0.0)
        validatorFuel.setExclusiveMin(True)
        self.lineEditFuel.setValidator(validatorFuel)

        validatorMM = QtPage.DoubleValidator(self.lineEditMassMolar, min=0.0)
        validatorMM.setExclusiveMin(True)
        self.lineEditMassMolar.setValidator(validatorMM)

        # Display

        model = self.mdl.getParticularPhysical()

        if model == "atmo":
            self.groupBoxTemperature.show()
            self.labelInfoT0.hide()
            self.groupBoxMassMolar.hide()
        elif model == "comp" or model == "coal":
            self.groupBoxTemperature.show()
            self.groupBoxMassMolar.show()
        elif model != "off":
            self.groupBoxTemperature.show()
            self.groupBoxMassMolar.hide()
        else:
            self.groupBoxTemperature.hide()
            self.groupBoxMassMolar.hide()

        gas_comb = GasCombustionModel(self.case).getGasCombustionModel()
        if gas_comb == 'd3p':
            self.groupBoxTempd3p.show()
            t_oxy  = self.mdl.getTempOxydant()
            t_fuel = self.mdl.getTempFuel()
            self.lineEditOxydant.setText(QString(str(t_oxy)))
            self.lineEditFuel.setText(QString(str(t_fuel)))
        else:
            self.groupBoxTempd3p.hide()

        # Initialization

        p = self.mdl.getPressure()
        self.lineEditP0.setText(QString(str(p)))

        v = self.mdl.getVelocity()
        self.lineEditV0.setText(QString(str(v)))

        init_length_choice = self.mdl.getLengthChoice()
        self.modelLength.setItem(str_model=init_length_choice)
        if init_length_choice == 'automatic':
            self.lineEditL0.setText(QString(str()))
            self.lineEditL0.setDisabled(True)
        else:
            self.lineEditL0.setEnabled(True)
            l = self.mdl.getLength()
            self.lineEditL0.setText(QString(str(l)))

        model = self.mdl.getParticularPhysical()
        if model == "atmo":
            t = self.mdl.getTemperature()
            self.lineEditT0.setText(QString(str(t)))
        elif model != "off":
            t = self.mdl.getTemperature()
            self.lineEditT0.setText(QString(str(t)))
            m = self.mdl.getMassemol()
            self.lineEditMassMolar.setText(QString(str(m)))

        self.case.undoStartGlobal()


    @pyqtSignature("const QString&")
    def slotPressure(self,  text):
        """
        Input PRESS.
        """
        p, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setPressure(p)


    @pyqtSignature("const QString&")
    def slotVelocity(self,  text):
        """
        Input Velocity.
        """
        v, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setVelocity(v)


    @pyqtSignature("const QString &")
    def slotLengthChoice(self,text):
        """
        Set value for parameterNTERUP
        """
        choice = self.modelLength.dicoV2M[str(text)]
        self.mdl.setLengthChoice(choice)
        if choice == 'automatic':
            self.lineEditL0.setText(QString(str()))
            self.lineEditL0.setDisabled(True)
        else:
            self.lineEditL0.setEnabled(True)
            value = self.mdl.getLength()
            self.lineEditL0.setText(QString(str(value)))
        log.debug("slotlengthchoice-> %s" % choice)


    @pyqtSignature("const QString&")
    def slotLength(self,  text):
        """
        Input reference length.
        """
        l, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setLength(l)


    @pyqtSignature("const QString&")
    def slotTemperature(self,  text):
        """
        Input TEMPERATURE.
        """
        t, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setTemperature(t)


    @pyqtSignature("const QString&")
    def slotTempOxydant(self,  text):
        """
        Input oxydant TEMPERATURE.
        """
        t, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setTempOxydant(t)


    @pyqtSignature("const QString&")
    def slotTempFuel(self,  text):
        """
        Input fuel TEMPERATURE.
        """
        t, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setTempFuel(t)


    @pyqtSignature("const QString&")
    def slotMassemol(self,  text):
        """
        Input Mass molar.
        """
        m, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.mdl.setMassemol(m)


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# Testing part
#-------------------------------------------------------------------------------

if __name__ == "__main__":

    pass

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
