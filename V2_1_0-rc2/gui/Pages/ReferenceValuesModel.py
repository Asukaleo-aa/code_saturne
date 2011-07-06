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
This module defines the values of reference.

This module contains the following classes and function:
- ReferenceValuesModel
- ReferenceValuesTestCase
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import sys, unittest

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Common import *
import Base.Toolbox as Tool
from Base.XMLvariables import Variables, Model
from Base.XMLmodel import ModelTest
from Pages.CoalCombustionModel import CoalCombustionModel
from Pages.GasCombustionModel import GasCombustionModel
from Pages.ElectricalModelsModel import ElectricalModel
from Pages.AtmosphericFlowsModel import AtmosphericFlowsModel

#-------------------------------------------------------------------------------
# Reference values model class
#-------------------------------------------------------------------------------

class ReferenceValuesModel(Model):
    """
    Manage the input/output markups in the xml doc about Pressure
    """
    def __init__(self, case):
        """
        Constructor.
        """
        self.case = case

        self.node_models = self.case.xmlGetNode('thermophysical_models')
        self.node_veloce = self.node_models.xmlGetNode('velocity_pressure')
        self.node_coal = self.node_models.xmlGetNode('pulverized_coal', 'model')
        self.node_gas   = self.node_models.xmlGetNode('gas_combustion',  'model')
        self.node_joule = self.node_models.xmlGetNode('joule_effect',  'model')
        self.node_atmo = self.node_models.xmlGetNode('atmospheric_flows',  'model')


    def defaultValues(self):
        """
        Return reference values by default
        """
        default = {}
        default['reference_pressure'] = 1.01325e+5
        default['reference_temperature'] = 1273.15
        if self.getParticularPhysical()[0] == "atmo":
            default['reference_temperature'] = 293.15
        # mass molar for dry air
        default['reference_mass_molar'] = 28.966e-3

        return default


    def setPressure(self, value):
        """
        Set value of reference pressure into xml file.
        """
        self.isGreaterOrEqual(value, 0.0)
        node = self.node_veloce.xmlGetNode('variable', name ='pressure')
        node.xmlSetData('reference_pressure', value)


    def getPressure(self):
        """
        Return the value of reference pressure.
        """
        node = self.node_veloce.xmlGetNode('variable', name ='pressure')
        value = node.xmlGetDouble('reference_pressure')
        if value == None:
            value = self.defaultValues()['reference_pressure']
            self.setPressure(value)

        return value


    def setTemperature(self, value):
        """
        Set reference temperature.
        """
        self.isGreater(value, 0.0)
        model, node = self.getParticularPhysical()
        node.xmlSetData('reference_temperature', value)


    def getTemperature(self):
        """
        Get reference temperature.
        """
        model, node = self.getParticularPhysical()
        value = node.xmlGetDouble('reference_temperature')
        if not value :
            value = self.defaultValues()['reference_temperature']
            self.setTemperature(value)
        return value


    def setMassemol(self, value):
        """
        Set reference mass molar.
        """
        self.isGreater(value, 0.0)
        model, node = self.getParticularPhysical()
        node.xmlSetData('reference_mass_molar', value)


    def getMassemol(self):
        """
        Get reference mass molar.
        """
        model, node = self.getParticularPhysical()
        value = node.xmlGetDouble('reference_mass_molar')
        if not value :
            value = self.defaultValues()['reference_mass_molar']
            self.setMassemol(value)
        return value


    def getParticularPhysical(self):
        """
        Get model for set temperature for relative model
        """
        model = 'off'
        node = None

        coalModel = CoalCombustionModel(self.case).getCoalCombustionModel()
        gasModel = GasCombustionModel(self.case).getGasCombustionModel()
        jouleModel = ElectricalModel(self.case).getElectricalModel()
        atmoModel = AtmosphericFlowsModel(self.case).getAtmosphericFlowsModel()

        if coalModel != 'off':
            model = "coal"
            node = self.node_coal
        elif gasModel != 'off':
            model = "gas"
            node = self.node_gas
        elif jouleModel != 'off':
            model = "joule"
            node = self.node_joule
        elif atmoModel != 'off':
            model = "atmo"
            node = self.node_atmo

        return model, node


#-------------------------------------------------------------------------------
# ReferenceValuesModel test case
#-------------------------------------------------------------------------------

class ReferenceValuesTestCase(ModelTest):
    """
    """
    def checkReferenceValuesInstantiation(self):
        """Check whether the ReferenceValuesModel class could be instantiated"""
        model = None
        model = ReferenceValuesModel(self.case)
        assert model != None, 'Could not instantiate ReferenceValuesModel'

    def checkGetandSetPressure(self):
        """Check whether the ReferenceValuesModel class could be set and get Pressure"""
        mdl = ReferenceValuesModel(self.case)
        mdl.setPressure(13e+5)

        doc = """<velocity_pressure>
                    <variable label="Pressure" name="pressure">
                            <reference_pressure>1.3e+06</reference_pressure>
                    </variable>
                    <variable label="VelocitU" name="velocity_U"/>
                    <variable label="VelocitV" name="velocity_V"/>
                    <variable label="VelocitW" name="velocity_W"/>
                    <property label="total_pressure" name="total_pressure"/>
                    <property label="Yplus" name="yplus" support="boundary"/>
                    <property label="Efforts" name="effort" support="boundary"/>
                    <property label="all_variables" name="all_variables" support="boundary"/>
                </velocity_pressure>"""
        assert mdl.node_veloce == self.xmlNodeFromString(doc),\
            'Could not set pressure ReferenceValuesModel'
        assert mdl.getPressure() == 13e+5,\
            'Could not get pressure ReferenceValuesModel'

    def checkGetandSetTemperature(self):
        """Check whether the ReferenceValuesModel class could be set and get Temperature"""
        mdl = ReferenceValuesModel(self.case)
        from Pages.CoalCombustionModel import CoalCombustionModel
        CoalCombustionModel(self.case).setCoalCombustionModel('coal_homo')
        del CoalCombustionModel
        mdl.setTemperature(55.5)

        doc = """<pulverized_coal model="coal_homo">
                    <scalar label="Enthalpy" name="Enthalpy" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="NP_CP01" name="NP_CP01" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="XCH_CP01" name="XCH_CP01" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="XCK_CP01" name="XCK_CP01" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="ENT_CP01" name="ENT_CP01" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="Fr_MV101" name="Fr_MV101" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="Fr_MV201" name="Fr_MV201" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="Fr_HET" name="Fr_HET" type="model"><flux_reconstruction status="off"/></scalar>
                    <scalar label="Var_AIR" name="Var_AIR" type="model"><flux_reconstruction status="off"/></scalar>
                    <property label="Temp_GAZ" name="Temp_GAZ"/>
                    <property label="ROM_GAZ" name="ROM_GAZ"/>
                    <property label="YM_CHx1m" name="YM_CHx1m"/>
                    <property label="YM_CHx2m" name="YM_CHx2m"/>
                    <property label="YM_CO" name="YM_CO"/>
                    <property label="YM_O2" name="YM_O2"/>
                    <property label="YM_CO2" name="YM_CO2"/>
                    <property label="YM_H2O" name="YM_H2O"/>
                    <property label="YM_N2" name="YM_N2"/>
                    <property label="XM" name="XM"/>
                    <property label="Temp_CP01" name="Temp_CP01"/>
                    <property label="Frm_CP01" name="Frm_CP01"/>
                    <property label="Rho_CP01" name="Rho_CP01"/>
                    <property label="Dia_CK01" name="Dia_CK01"/>
                    <property label="Ga_DCH01" name="Ga_DCH01"/>
                    <property label="Ga_DV101" name="Ga_DV101"/>
                    <property label="Ga_DV201" name="Ga_DV201"/>
                    <property label="Ga_HET01" name="Ga_HET01"/>
                    <property label="ntLuminance_4PI" name="ntLuminance_4PI"/>
                    <reference_temperature>55.5</reference_temperature>
                 </pulverized_coal>"""
        assert mdl.node_coal == self.xmlNodeFromString(doc),\
            'Could not set temperature ReferenceValuesModel'
        assert mdl.getTemperature() == 55.5,\
            'Could not get temperature ReferenceValuesModel'

    def checkGetandSsetMassemol(self):
        """Check whether the ReferenceValuesModel class could be set and get Molar mass"""
        mdl = ReferenceValuesModel(self.case)
        from Pages.GasCombustionModel import GasCombustionModel
        GasCombustionModel(self.case).setGasCombustionModel('ebu')
        del GasCombustionModel
        mdl.setMassemol(50.8e-3)
        doc = """<gas_combustion model="ebu">
                    <reference_mass_molar>0.0508</reference_mass_molar>
                 </gas_combustion>"""
        assert mdl.node_gas == self.xmlNodeFromString(doc),\
            'Could not set molar mass ReferenceValuesModel'
        assert mdl.getMassemol() == 50.8e-3,\
            'Could not get molar mass ReferenceValuesModel'


def suite():
    testSuite = unittest.makeSuite(ReferenceValuesTestCase, "check")
    return testSuite

def runTest():
    print("ReferenceValuesTestCase")
    runner = unittest.TextTestRunner()
    runner.run(suite())

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
