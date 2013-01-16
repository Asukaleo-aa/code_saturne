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
- BoundaryConditionsTurbulenceInletView
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

from Pages.BoundaryConditionsTurbulenceInletForm import Ui_BoundaryConditionsTurbulenceInletForm
from Pages.TurbulenceModel import TurbulenceModel

from Base.Toolbox import GuiParam
from Base.QtPage import DoubleValidator, ComboModel, setGreenColor
from Pages.QMeiEditorView import QMeiEditorView

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("BoundaryConditionsTurbulenceInletView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class BoundaryConditionsTurbulenceInletView(QWidget, Ui_BoundaryConditionsTurbulenceInletForm):
    """
    Boundary condition for turbulence
    """
    def __init__(self, parent):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_BoundaryConditionsTurbulenceInletForm.__init__(self)
        self.setupUi(self)


    def setup(self, case):
        """
        Setup the widget
        """
        self.__case = case
        self.__boundary = None

        self.__case.undoStopGlobal()

        self.connect(self.comboBoxTurbulence, SIGNAL("activated(const QString&)"), self.__slotChoiceTurbulence)

        self.__modelTurbulence = ComboModel(self.comboBoxTurbulence, 2, 1)
        self.__modelTurbulence.addItem(self.tr("Calculation by hydraulic diameter"), 'hydraulic_diameter')
        self.__modelTurbulence.addItem(self.tr("Calculation by turbulent intensity"), 'turbulent_intensity')
        self.__modelTurbulence.addItem(self.tr("Calculation by formula"), 'formula')

        self.connect(self.lineEditDiameter, SIGNAL("textChanged(const QString &)"), self.__slotDiam)
        self.connect(self.lineEditIntensity, SIGNAL("textChanged(const QString &)"), self.__slotIntensity)
        self.connect(self.lineEditDiameterIntens, SIGNAL("textChanged(const QString &)"), self.__slotDiam)
        self.connect(self.pushButtonTurb, SIGNAL("clicked()"), self.__slotTurbulenceFormula)

        validatorDiam = DoubleValidator(self.lineEditDiameter, min=0.)
        validatorDiam.setExclusiveMin(True)
        validatorIntensity = DoubleValidator(self.lineEditIntensity, min=0.)

        self.lineEditDiameter.setValidator(validatorDiam)
        self.lineEditDiameterIntens.setValidator(validatorDiam)
        self.lineEditIntensity.setValidator(validatorIntensity)

        self.__case.undoStartGlobal()


    def showWidget(self, boundary):
        """
        Show the widget
        """
        self.__boundary = boundary

        if TurbulenceModel(self.__case).getTurbulenceVariable():
            turb_choice = boundary.getTurbulenceChoice()
            self.__modelTurbulence.setItem(str_model=turb_choice)
            if turb_choice == "hydraulic_diameter":
                self.frameTurbDiameter.show()
                self.frameTurbIntensity.hide()
                self.pushButtonTurb.setEnabled(False)
                setGreenColor(self.pushButtonTurb, False)
                d = boundary.getHydraulicDiameter()
                self.lineEditDiameter.setText(QString(str(d)))
            elif turb_choice == "turbulent_intensity":
                self.frameTurbIntensity.show()
                self.frameTurbDiameter.hide()
                self.pushButtonTurb.setEnabled(False)
                setGreenColor(self.pushButtonTurb, False)
                i = boundary.getTurbulentIntensity()
                d = boundary.getHydraulicDiameter()
                self.lineEditIntensity.setText(QString(str(i)))
                self.lineEditDiameterIntens.setText(QString(str(d)))
            elif turb_choice == "formula":
                self.frameTurbIntensity.hide()
                self.frameTurbDiameter.hide()
                self.pushButtonTurb.setEnabled(True)
                setGreenColor(self.pushButtonTurb, True)
            self.show()
        else:
            self.hideWidget()


    def hideWidget(self):
        """
        Hide the widget
        """
        self.hide()


    @pyqtSignature("const QString&")
    def __slotChoiceTurbulence(self, text):
        """
        INPUT choice of method of calculation of the turbulence
        """
        turb_choice = self.__modelTurbulence.dicoV2M[str(text)]
        self.__boundary.setTurbulenceChoice(turb_choice)

        self.frameTurbDiameter.hide()
        self.frameTurbIntensity.hide()
        self.pushButtonTurb.setEnabled(False)
        setGreenColor(self.pushButtonTurb, False)

        if turb_choice  == 'hydraulic_diameter':
            self.frameTurbDiameter.show()
            d = self.__boundary.getHydraulicDiameter()
            self.lineEditDiameter.setText(QString(str(d)))
        elif turb_choice == 'turbulent_intensity':
            self.frameTurbIntensity.show()
            i = self.__boundary.getTurbulentIntensity()
            self.lineEditIntensity.setText(QString(str(i)))
            d = self.__boundary.getHydraulicDiameter()
            self.lineEditDiameterIntens.setText(QString(str(d)))
        elif turb_choice == 'formula':
            self.pushButtonTurb.setEnabled(True)
            setGreenColor(self.pushButtonTurb, True)


    @pyqtSignature("const QString&")
    def __slotDiam(self, text):
        """
        INPUT hydraulic diameter
        """
        diam, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setHydraulicDiameter(diam)


    @pyqtSignature("const QString&")
    def __slotIntensity(self, text):
        """
        INPUT turbulent intensity
        """
        intens, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setTurbulentIntensity(intens)


    @pyqtSignature("")
    def __slotTurbulenceFormula(self):
        """
        INPUT user formula
        """
        turb_model = TurbulenceModel(self.__case).getTurbulenceModel()
        if turb_model in ('k-epsilon', 'k-epsilon-PL'):

            exp = self.__boundary.getTurbFormula()
            if not exp:
                exp = self.__boundary.getDefaultTurbFormula(turb_model)
            exa = """#example :
uref2 = 10.;
dh = 0.2;
re = sqrt(uref2)*dh*rho0/mu0;

if (re < 2000){
#     in this case u*^2 is directly calculated to not have a problem with
#     xlmbda=64/Re when Re->0

  ustar2 = 8.*mu0*sqrt(uref2)/rho0/dh;}

else if (re<4000){

  xlmbda = 0.021377 + 5.3115e-6*re;
  ustar2 = uref2*xlmbda/8.;}

else {

  xlmbda = 1/( 1.8*log(re)/log(10.)-1.64)^2;
  ustar2 = uref2*xlmbda/8.;}

cmu = 0.09;
kappa = 0.42;
k   = ustar2/sqrt(cmu);
eps = ustar2^1.5/(kappa*dh*0.1);"""
            req = [('k', "turbulent energy"),
            ('eps', "turbulent dissipation")]
            sym = [('x','cell center coordinate'),
                   ('y','cell center coordinate'),
                   ('z','cell center coordinate'),
                   ('t','time'),
                   ('dt','time step'),
                   ('iter','number of time step')]
            dialog = QMeiEditorView(self,
                                    check_syntax = self.__case['package'].get_check_syntax(),
                                    expression = exp,
                                    required   = req,
                                    symbols    = sym,
                                    examples   = exa)
            if dialog.exec_():
                result = dialog.get_result()
                log.debug("slotFormulaTurb -> %s" % str(result))
                self.__boundary.setTurbFormula(result)
                setGreenColor(self.pushButtonTurb, False)

        elif turb_model in ('Rij-epsilon', 'Rij-SSG'):

            exp = self.__boundary.getTurbFormula()
            if not exp:
                exp = self.__boundary.getDefaultTurbFormula(turb_model)
            exa = """#exemple :
uref2 = 10.;
dh = 0.2;
re = sqrt(uref2)*dh*rho0/mu0;

if (re < 2000){
#     in this case u*^2 is directly calculated to not have a problem with
#     xlmbda=64/Re when Re->0

  ustar2 = 8.*mu0*sqrt(uref2)/rho0/dh;}

else if (re<4000){

  xlmbda = 0.021377 + 5.3115e-6*re;
  ustar2 = uref2*xlmbda/8.;}

else {

  xlmbda = 1/( 1.8*log(re)/log(10.)-1.64)^2;
  ustar2 = uref2*xlmbda/8.;}

cmu = 0.09;
kappa = 0.42;
k   = ustar2/sqrt(cmu);
eps = ustar2^1.5/(kappa*dh*0.1);
d2s3 = 2/3;
R11 = d2s3*k;
R22 = d2s3*k;
R33 = d2s3*k;
R12 = 0;
R13 = 0;
R23 = 0;
"""
            req = [('R11', "Reynolds stress R11"),
            ('R22', "Reynolds stress R22"),
            ('R33', "Reynolds stress R33"),
            ('R12', "Reynolds stress R12"),
            ('R13', "Reynolds stress R13"),
            ('R23', "Reynolds stress R23"),
            ('eps', "turbulent dissipation")]
            sym = [('x','cell center coordinate'),
                   ('y','cell center coordinate'),
                   ('z','cell center coordinate'),
                   ('t','time'),
                   ('dt','time step'),
                   ('iter','number of time step')]
            dialog = QMeiEditorView(self,
                                    check_syntax = self.__case['package'].get_check_syntax(),
                                    expression = exp,
                                    required   = req,
                                    symbols    = sym,
                                    examples   = exa)
            if dialog.exec_():
                result = dialog.get_result()
                log.debug("slotFormulaTurb -> %s" % str(result))
                self.__boundary.setTurbFormula(result)
                setGreenColor(self.pushButtonTurb, False)

        elif turb_model == 'Rij-EBRSM':

            exp = self.__boundary.getTurbFormula()
            if not exp:
                exp = self.__boundary.getDefaultTurbFormula(turb_model)
            exa = """#exemple :
uref2 = 10.;
dh = 0.2;
re = sqrt(uref2)*dh*rho0/mu0;

if (re < 2000){
#     in this case u*^2 is directly calculated to not have a problem with
#     xlmbda=64/Re when Re->0

  ustar2 = 8.*mu0*sqrt(uref2)/rho0/dh;}

else if (re<4000){

  xlmbda = 0.021377 + 5.3115e-6*re;
  ustar2 = uref2*xlmbda/8.;}

else {

  xlmbda = 1/( 1.8*log(re)/log(10.)-1.64)^2;
  ustar2 = uref2*xlmbda/8.;}

cmu = 0.09;
kappa = 0.42;
k   = ustar2/sqrt(cmu);
eps = ustar2^1.5/(kappa*dh*0.1);
d2s3 = 2/3;
R11 = d2s3*k;
R22 = d2s3*k;
R33 = d2s3*k;
R12 = 0;
R13 = 0;
R23 = 0;
alpha =  1.;
"""
            req = [('R11', "Reynolds stress R11"),
            ('R22', "Reynolds stress R22"),
            ('R33', "Reynolds stress R33"),
            ('R12', "Reynolds stress R12"),
            ('R13', "Reynolds stress R13"),
            ('R23', "Reynolds stress R23"),
            ('eps', "turbulent dissipation"),
            ('alpha', "alpha")]
            sym = [('x','cell center coordinate'),
                   ('y','cell center coordinate'),
                   ('z','cell center coordinate'),
                   ('t','time'),
                   ('dt','time step'),
                   ('iter','number of time step')]
            dialog = QMeiEditorView(self,
                                    check_syntax = self.__case['package'].get_check_syntax(),
                                    expression = exp,
                                    required   = req,
                                    symbols    = sym,
                                    examples   = exa)
            if dialog.exec_():
                result = dialog.get_result()
                log.debug("slotFormulaTurb -> %s" % str(result))
                self.__boundary.setTurbFormula(result)
                setGreenColor(self.pushButtonTurb, False)

        elif turb_model == 'v2f-BL-v2/k':

            exp = self.__boundary.getTurbFormula()
            if not exp:
                exp = self.__boundary.getDefaultTurbFormula(turb_model)
            exa = """#exemple :
uref2 = 10.;
dh = 0.2;
re = sqrt(uref2)*dh*rho0/mu0;

if (re < 2000){
#     in this case u*^2 is directly calculated to not have a problem with
#     xlmbda=64/Re when Re->0

  ustar2 = 8.*mu0*sqrt(uref2)/rho0/dh;}

else if (re<4000){

  xlmbda = 0.021377 + 5.3115e-6*re;
  ustar2 = uref2*xlmbda/8.;}

else {

  xlmbda = 1/( 1.8*log(re)/log(10.)-1.64)^2;
  ustar2 = uref2*xlmbda/8.;}

cmu = 0.09;
kappa = 0.42;
d2s3 = 2/3;
k   = ustar2/sqrt(cmu);
eps = ustar2^1.5/(kappa*dh*0.1);
phi = d2s3;
alpha = 0;"""
            req = [('k', "turbulent energy"),
            ('eps', "turbulent dissipation"),
            ('phi', "variable phi in v2f model"),
            ('alpha', "variable alpha in v2f model")]
            sym = [('x','cell center coordinate'),
                   ('y','cell center coordinate'),
                   ('z','cell center coordinate'),
                   ('t','time'),
                   ('dt','time step'),
                   ('iter','number of time step')]
            dialog = QMeiEditorView(self,
                                    check_syntax = self.__case['package'].get_check_syntax(),
                                    expression = exp,
                                    required   = req,
                                    symbols    = sym,
                                    examples   = exa)
            if dialog.exec_():
                result = dialog.get_result()
                log.debug("slotFormulaTurb -> %s" % str(result))
                self.__boundary.setTurbFormula(result)
                setGreenColor(self.pushButtonTurb, False)

        elif turb_model == 'k-omega-SST':

            exp = self.__boundary.getTurbFormula()
            if not exp:
                exp = self.__boundary.getDefaultTurbFormula(turb_model)
            exa = """#exemple :
uref2 = 10.;
dh = 0.2;
re = sqrt(uref2)*dh*rho0/mu0;

if (re < 2000){
#     in this case u*^2 is directly calculated to not have a problem with
#     xlmbda=64/Re when Re->0

  ustar2 = 8.*mu0*sqrt(uref2)/rho0/dh;}

else if (re<4000){

  xlmbda = 0.021377 + 5.3115e-6*re;
  ustar2 = uref2*xlmbda/8.;}

else {

  xlmbda = 1/( 1.8*log(re)/log(10.)-1.64)^2;
  ustar2 = uref2*xlmbda/8.;}

cmu = 0.09;
kappa = 0.42;
k   = ustar2/sqrt(cmu);
eps = ustar2^1.5/(kappa*dh*0.1);
omega = eps/(cmu * k);"""
            req = [('k', "turbulent energy"),
            ('omega', "specific dissipation rate")]
            sym = [('x','cell center coordinate'),
                   ('y','cell center coordinate'),
                   ('z','cell center coordinate'),
                   ('t','time'),
                   ('dt','time step'),
                   ('iter','number of time step')]
            dialog = QMeiEditorView(self,
                                    check_syntax = self.__case['package'].get_check_syntax(),
                                    expression = exp,
                                    required   = req,
                                    symbols    = sym,
                                    examples   = exa)
            if dialog.exec_():
                result = dialog.get_result()
                log.debug("slotFormulaTurb -> %s" % str(result))
                self.__boundary.setTurbFormula(result)
                setGreenColor(self.pushButtonTurb, False)

        elif turb_model == 'Spalart-Allmaras':

            exp = self.__boundary.getTurbFormula()
            if not exp:
                exp = self.__boundary.getDefaultTurbFormula(turb_model)
            exa = """#exemple :
uref2 = 10.;
dh = 0.2;
re = sqrt(uref2)*dh*rho0/mu0;

if (re < 2000){
#     in this case u*^2 is directly calculated to not have a problem with
#     xlmbda=64/Re when Re->0

  ustar2 = 8.*mu0*sqrt(uref2)/rho0/dh;}

else if (re<4000){

  xlmbda = 0.021377 + 5.3115e-6*re;
  ustar2 = uref2*xlmbda/8.;}

else {

  xlmbda = 1/( 1.8*log(re)/log(10.)-1.64)^2;
  ustar2 = uref2*xlmbda/8.;}

cmu = 0.09;
kappa = 0.42;
k   = ustar2/sqrt(cmu);
eps = ustar2^1.5/(kappa*dh*0.1);
nusa = eps/(cmu * k);"""
            req = [('nusa', "nusa")]
            sym = [('x','cell center coordinate'),
                   ('y','cell center coordinate'),
                   ('z','cell center coordinate'),
                   ('t','time'),
                   ('dt','time step'),
                   ('iter','number of time step')]
            dialog = QMeiEditorView(self,
                                    check_syntax = self.__case['package'].get_check_syntax(),
                                    expression = exp,
                                    required   = req,
                                    symbols    = sym,
                                    examples   = exa)
            if dialog.exec_():
                result = dialog.get_result()
                log.debug("slotFormulaTurb -> %s" % str(result))
                self.__boundary.setTurbFormula(result)
                setGreenColor(self.pushButtonTurb, False)


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
