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
This module defines the Thermal Radiation model

This module contains the following classes and function:
- ThermalRadiationModel
- ThermalRadiationTestCase
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import unittest

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Common import *
from Base.XMLmodel import ModelTest
from Base.XMLvariables import Variables, Model
from Pages.OutputControlModel import OutputControlModel

#-------------------------------------------------------------------------------
# ThermalRadiation model class
#-------------------------------------------------------------------------------

class ThermalRadiationModel(Model):

    def __init__(self, case):
        """
        Constuctor
        """
        self.case = case

        self.node_models = self.case.xmlGetNode('thermophysical_models')
        self.node_ray    = self.node_models.xmlInitNode('radiative_transfer')
        self.node_Coal   = self.node_models.xmlGetNode('pulverized_coal')

        self.radiativeModels = ('off', 'dom', 'p-1')
        self.optionsList = [0, 1, 2]

        self.c_prop = {}
        self.b_prop = {}

        #self.c_prop['intensity']              = self.tr("Intensity")
        #self.c_prop['implicite_source_term']  = self.tr("Implicite_source_term")
        self.c_prop['qrad_x']                 = self.tr("Qrad_x")
        self.c_prop['qrad_y']                 = self.tr("Qrad_y")
        self.c_prop['qrad_z']                 = self.tr("Qrad_z")
        self.c_prop['radiative_source_term']  = self.tr("Radiative_source_term")
        self.c_prop['absorption']             = self.tr("Absorption")
        self.c_prop['emission']               = self.tr("Emission")
        self.c_prop['absorption_coefficient'] = self.tr("Absorption_coefficient")

        self.b_prop['wall_temp']              = self.tr("Wall_temperature")
        self.b_prop['flux_incident']          = self.tr("Flux_incident")
        self.b_prop['thickness']              = self.tr("Thickness")
        self.b_prop['thermal_conductivity']   = self.tr("Thermal_conductivity")
        self.b_prop['emissivity']             = self.tr("Emissivity")
        self.b_prop['flux_net']               = self.tr("Flux_net")
        self.b_prop['flux_convectif']         = self.tr("Flux_convectif")
        self.b_prop['coeff_ech_conv']         = self.tr("Coeff_ech_convectif")

        self.classesNumber = 0
        self.isCoalCombustion()


    def __volumeProperties(self):
        """
        Return the name and the defaul label for cells properties.
        """
        for k, v in self.c_prop.items():
            if k in ('absorption', 'emission', 'radiative_source_term', 'absorption_coefficient'):
                for classe in range(1, self.classesNumber+1):
                    k = '%s_%2.2i' % (k, classe)
                    v = '%s_%2.2i' % (v, classe)
                    self.c_prop[k] = v
        return self.c_prop


    def __boundaryProperties(self):
        """
        Return the name and the defaul label for boundaries properties.
        """
        return self.b_prop


    def _defaultValues(self):
        """
        Private method : return in a dictionnary which contains default values
        """
        default = {}
        default['radiative_model']   = "off"
        default['directions_number'] = 32
        default['restart_status']    = 'off'
        default['type_coef']         = 'constant'
        default['value_coef']        = 0.0
        default['frequency']         = 1
        default['idiver']            = 2
        default['tempP']             = 1
        default['intensity']         = 0
        return default


    def __dicoRayLabel():
        """
        """
        dico = {}
        rayName = ['srad',     'qrad',     'absorp',  'emiss',    'coefAb',
                    'wall_temp', 'flux_incident', 'thermal_conductivity', 'thickness',
                    'emissivity', 'flux_net',      'flux_convectif',  'coeff_ech_conv']

        raylabF = ['Srad',       'Qrad',          'Absorp',     'Emiss',    'CoefAb',
                    'Temp_paroi', 'Flux_incident', 'Conductivite_th', 'Epaisseur',
                'Emissivite', 'Flux_net',      'Flux_convectif',  'Coeff_ech_conv']

        raylabE = ['Srad',      'Qrad',      'Absorp',          'Emiss',    'CoefAb',
                    'Wall_temp', 'Flux_incident', 'Th_conductivity', 'Thickness',
                'Emissivity','Flux_net',      'Flux_convectif',  'Coeff_ech_conv']

        dico['name'] = rayName
        dico['labF'] = raylabF
        dico['labE'] = raylabE
        if GuiParam.lang == 'fr':
            label = dico['labF']
        else:
            label = dico['labE']

        return dico['name'], label


#    def _getCoalModel(self):
#        """
#        Private method : return value model of radiative transfer for coal
#        """
#        import Pages.CoalThermoChemistry as CoalThermoChemistry
#        model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
#        model.load()
#        ind = model.radiativTransfer.getRadiativTransfer()
#        if (ind == 1) or (ind == 2):
#            val = "dom"
#        elif (ind == 3) or (ind == 4):
#            val = "p-1"
#        else:
#            val = self._defaultValues()['radiative_model']
#        return val


#    def _setCoalModel(self, model2):
#        """
#        Private method : put value of model of radiative transfer for coal
#        """
#        self.isInList(model2, self.radiativeModels)
#        import Pages.CoalThermoChemistry as CoalThermoChemistry
#        model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
#        model.load()
#        ind = 0
#        node_coeff = self.node_ray.xmlGetNode('absorption_coefficient', 'type')
#        if model2 != "off":
#            if model2 == "p-1":
#                ind = 2
#            coalCoeff = 'constant'
#            if node_coeff:
#                coalCoeff = node_coeff['type']
#            if coalCoeff == 'constant':
#                ind += 1
#            elif coalCoeff == 'modak':
#                ind += 2
#        model.radiativTransfer.setRadiativTransfer(ind)
#        model.save()


#    def _getTypeCoalCoeff(self):
#        """
#        Private method : return type of coefficient absorption for coal
#        """
#        import Pages.CoalThermoChemistry as CoalThermoChemistry
#        model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
#        model.load()
#        ind = model.radiativTransfer.getRadiativTransfer()
#        type = "constant"
#        if (ind == 1) or (ind == 3):
#            type = "constant"
#        elif (ind == 2) or (ind == 4):
#            type = "modak"
#        return type


#    def _setTypeCoalCoeff(self, val):
#        """
#        Private method : put indice relatively to type of
#        coefficient absorption for coal
#        """
#        self.isInList(val, ('constant', 'variable', 'modak'))
#        import Pages.CoalThermoChemistry as CoalThermoChemistry
#        model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
#        model.load()
#        if val == "constant":
#            ind = 1
#        else :
#            ind = 2
#
#        radModel = self.node_ray['model']
#        if radModel == 'p-1':
#            ind += 2
#        elif radModel == 'off':
#            ind = 0
#        model.radiativTransfer.setRadiativTransfer(ind)
#        model.save()


    def _setBoundCond(self):
        """
        Private method : put by default boundary conditions for radiative
        variables as soon as a radiative model is set
        """
        from Pages.LocalizationModel import LocalizationModel, Zone
        from Pages.Boundary import Boundary
        d = LocalizationModel('BoundaryZone', self.case)
        for zone in d.getZones():
            nature = zone.getNature()
            if nature == 'wall':
                label = zone.getLabel()
                bdModel = Boundary("radiative_wall", label, self.case)
                bdModel.getRadiativeChoice()


    def _updateModelParameters(self, model):
        """
        Private method : put by default all parameters for radiative
        variables as soon a radiative model is set
        """
        self.getRestart()
        if model == 'dom':
            self.getNbDir()
        elif model == 'p-1':
            self.node_ray.xmlRemoveChild('directions_number')
        self.getAbsorCoeff()
        self.getFrequency()
        self.getTrs()
        self.getTemperatureListing()
        self.getIntensityResolution()


    def isCoalCombustion(self):
        """
        Return 0 if pulverized_coal's attribute model is 'off',
        return 1 if it's different
        """
        value = 0
        if self.node_Coal and self.node_Coal.xmlGetAttribute('model') != 'off':
            value = 1
            import Pages.CoalThermoChemistry as CoalThermoChemistry
            model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
            coalsNumber = model.getCoals().getCoalNumber()
            self.classesNumber = sum(model.getCoals().getClassesNumberList())

        return value


#    def _setVariable_ray(self):
#        """
#        Private method: put all variables for thermal radiative transfer
#        """
#        dico = self.__dicoRayLabel()
#        if self.getRadiativeModel() != "off":
#            for nb in range(len(dico[0])):
#                if not self.node_ray.xmlGetNode('property', name =dico[0][nb]):
#                    if dico[0][nb] in ("srad", "qrad", "absorp", "emiss", "coefAb"):
#                        self.node_ray.xmlInitNode('property',
#                                                   label=dico[1][nb],
#                                                   name =dico[0][nb])
#                    else:
#                        self.node_ray.xmlInitNode('property',
#                                                   label=dico[1][nb],
#                                                   name =dico[0][nb],
#                                                   support='boundary')


    def _setVariable_ray(self):
        if self.getRadiativeModel() != "off":
            for k, v in self.__volumeProperties().items():
                if not self.node_ray.xmlGetNode('property', name=k):
                    self.node_ray.xmlInitNode('property', label=v, name=k)

            for k, v in self.__boundaryProperties().items():
                if not self.node_ray.xmlGetNode('property', name=k):
                    self.node_ray.xmlInitNode('property', label=v, name=k, support='boundary')


    def getRadiativeModel(self):
        """
        Return value of attribute model
        """
        #if self.isCoalCombustion():
        #    return self._getCoalModel()
        #else:
        model = self.node_ray['model']
        if model not in self.radiativeModels:
            model = self._defaultValues()['radiative_model']
            self.setRadiativeModel(model)
        return model


    def setRadiativeModel(self, model):
        """
        Put value of attribute model to radiative transfer markup
        """
        self.isInList(model, self.radiativeModels)
        self.node_ray['model'] = model
        #if self.isCoalCombustion():
        #    self._setCoalModel(model)
        if model in ('dom', 'p-1'):
            self._setVariable_ray()
            self._setBoundCond()
            self._updateModelParameters(model)
            OutputControlModel(self.case).setDomainBoundaryPostProStatus('on')



    def getNbDir(self):
        """ Return value of number of directions """
        nb = self.node_ray.xmlGetInt('directions_number')
        if nb == None:
            nb = self._defaultValues()['directions_number']
            self.setNbDir(nb)
        return nb


    def setNbDir(self, val):
        """ Put value of number of directions """
        self.isIntInList(val, [32, 128])
        self.isInList(self.getRadiativeModel(), ('off', 'dom'))
        self.node_ray.xmlSetData('directions_number', val)


    def getRestart(self):
        """
        Return status of restart markup
        """
        node = self.node_ray.xmlInitNode('restart', 'status')
        status = node['status']
        if not status:
            status = self._defaultValues()['restart_status']
            self.setRestart(status)
        return status


    def setRestart(self, status):
        """
        Put status of restart markup
        """
        self.isOnOff(status)
        node = self.node_ray.xmlInitNode('restart', 'status')
        node['status'] = status


    def getTypeCoeff(self):
        """
        Return value of attribute type of 'absorption_coefficient' markup
        """
        node = self.node_ray.xmlInitNode('absorption_coefficient', 'type')
        type = node['type']
#        if self.isCoalCombustion():
#            type = self._getTypeCoalCoeff()
#            self._setTypeCoalCoeff(type)
#        else:
        if not type:
            type = self._defaultValues()['type_coef']
            self.setTypeCoeff(type)
        return type


    def setTypeCoeff(self, type):
        """
        Put value of attribute type of 'absorption_coefficient' markup
        """
        self.isInList(type, ('constant', 'variable', 'formula', 'modak'))
#        if self.isCoalCombustion():
#            self._setTypeCoalCoeff(type)
        node  = self.node_ray.xmlInitNode('absorption_coefficient', 'type')
        node['type'] = type


    def getAbsorCoeff(self):
        """
        Return value of absorption coefficient
        """
        if self.isCoalCombustion():
            import Pages.CoalThermoChemistry as CoalThermoChemistry
            model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
            model.load()
            val = model.radiativTransfer.getAbsorptionCoeff()
        else :
            self.getTypeCoeff()
            node = self.node_ray.xmlGetNode('absorption_coefficient', 'type')
            val = self.node_ray.xmlGetDouble('absorption_coefficient')
            if val == None:
                val = self._defaultValues()['value_coef']
                self.setAbsorCoeff(val)
        return val


    def setAbsorCoeff(self, val):
        """
        Put value of absorption coefficient
        """
        self.isPositiveFloat(val)
        if self.isCoalCombustion():
            import Pages.CoalThermoChemistry as CoalThermoChemistry
            model = CoalThermoChemistry.CoalThermoChemistryModel("dp_FCP", self.case)
            model.load()
            model.radiativTransfer.setAbsorptionCoeff(val)
            model.save()

        t = self.getTypeCoeff()
        self.node_ray.xmlSetData('absorption_coefficient', val, type=t)



    def getFrequency(self):
        """ Return value of frequency for advanced options """
        freq = self.node_ray.xmlGetInt('frequency')
        if freq == None:
            freq = self._defaultValues()['frequency']
            self.setFrequency(freq)
        return freq


    def setFrequency(self, val):
        """ Put value of frequency for advanced options """
        self.isInt(val)
        self.node_ray.xmlSetData('frequency', val)


    def getIntensityResolution(self):
        """ Return value of IIMLUM for advanced options """
        intens = self.node_ray.xmlGetInt('intensity_resolution_listing_printing')
        if intens == None:
            intens = self._defaultValues()['intensity']
            self.setIntensityResolution(intens)
        return intens


    def setIntensityResolution(self, intens):
        """ Put value of IIMLUM for advanced options """
        self.isIntInList(intens, self.optionsList)
        self.node_ray.xmlSetData('intensity_resolution_listing_printing', intens)


    def getTemperatureListing(self):
        """ Return value of IIMPAR for advanced options """
        tp = self.node_ray.xmlGetInt('temperature_listing_printing')
        if tp == None:
            tp = self._defaultValues()['tempP']
            self.setTemperatureListing(tp)
        return tp


    def setTemperatureListing(self, val):
        """ Put value of IIMPAR for advanced options """
        self.isIntInList(val, self.optionsList)
        self.node_ray.xmlSetData('temperature_listing_printing', val)


    def getTrs(self):
        """ Return value of IDIVER for advanced options """
        idiver = self.node_ray.xmlGetInt('thermal_radiative_source_term')
        if idiver == None:
            idiver = self._defaultValues()['idiver']
            self.setTrs(idiver)
        return idiver


    def setTrs(self, idiver):
        """ Put value of IDIVER for advanced options """
        self.isIntInList(idiver, self.optionsList)
        self.node_ray.xmlSetData('thermal_radiative_source_term', idiver)


    def getThermalRadiativeModel(self):
        """
        Return 0 if thermal radiative transfer is activated, or 1 if not.
        Only used by the view of Thermal Scalar for the tree management.
        """
        if self.node_ray['model'] != 'off':
            return 0
        else:
            return 1

    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# ThermalRadiation test case
#-------------------------------------------------------------------------------


class ThermalRadiationTestCase(ModelTest):
    """
    """
    def checkThermalRadiationInstantiation(self):
        """
        Check whether the ThermalRadiationModel class could be instantiated
        """
        mdl = None
        mdl = ThermalRadiationModel(self.case)

        assert mdl != None, 'Could not instantiate ThermalRadiationModel'


    def checkIsCoalCombustion(self):
        """
        Check whether the ThermalRadiationModel class could be in coal combustion
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.node_Coal['model'] = 'off'
        assert mdl.isCoalCombustion() != 1, 'Could not verify pulverized_coal is "on"'

    def checkSetandGetRadiativeModel(self):
        """
        Check whether the ThermalRadiationModel class could be set and get model
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0</absorption_coefficient>
                    <directions_number>32</directions_number>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc), \
            'Could not set model in ThermalRadiationModel'
        assert mdl.getRadiativeModel() == 'dom', \
            'Could not get model in ThermalRadiationModel'

    def checkSetandgetNbDir(self):
        """
        Check whether the ThermalRadiationModel class could be set and get
        number of directions
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setNbDir(128)
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0</absorption_coefficient>
                    <directions_number>128</directions_number>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc), \
            'Could not set number of directions in ThermalRadiationModel'
        assert mdl.getNbDir() == 128, \
            'Could not get number of directions in ThermalRadiationModel'

    def checkSetandGetRestart(self):
        """
        Check whether the ThermalRadiationModel class could be set and get restart
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setRestart('on')
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="on"/>
                    <absorption_coefficient type="constant">0</absorption_coefficient>
                    <directions_number>32</directions_number>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc), \
            'Could not set restart in ThermalRadiationModel'
        assert mdl.getRestart() == 'on', \
            'Could not get restart in ThermalRadiationModel'

    def checkSetandGetTypeCoeff(self):
        """
        Check whether the ThermalRadiationModel class could be set and
        get type of absorption coefficient
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setTypeCoeff('variable')
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="variable">0</absorption_coefficient>
                    <directions_number>32</directions_number>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc), \
            'Could not set type of absorption coefficient in ThermalRadiationModel'
        assert mdl.getTypeCoeff() == 'variable', \
            'Could not get type of absorption coefficient in ThermalRadiationModel'

    def checkSetandGetAbsorCoeff(self):
        """
        Check whether the ThermalRadiationModel class could be set and
        get value of absorption coefficient
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setAbsorCoeff(0.77)
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0.77</absorption_coefficient>
                    <directions_number>32</directions_number>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc),\
            'Could not set value of absorption coefficient in ThermalRadiationModel'
        assert mdl.getAbsorCoeff() == 0.77,\
            'Could not get value of absorption coefficient in ThermalRadiationModel'

    def checkSetandGetFrequency(self):
        """
        Check whether the ThermalRadiationModel class could be set and get
        frequency for advanced options
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setFrequency(12)
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0.0</absorption_coefficient>
                    <directions_number>32</directions_number>
                    <frequency>12</frequency>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc),\
            'Could not set frequency for advanced options in ThermalRadiationModel'
        assert mdl.getFrequency() == 12,\
            'Could not get frequency for advanced options in ThermalRadiationModel'

    def checkSetandGetIntensityResolution(self):
        """
        Check whether the ThermalRadiationModel class could be set and get
        IIMLUM for advanced options
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setIntensityResolution(1)
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0.0</absorption_coefficient>
                    <directions_number>32</directions_number>
                    <intensity_resolution_listing_printing>1</intensity_resolution_listing_printing>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc),\
            'Could not set IIMLUM for advanced options in ThermalRadiationModel'
        assert mdl.getFrequency() == 1,\
            'Could not get IIMLUM for advanced options in ThermalRadiationModel'

    def checkSetandGetTemperatureListing(self):
        """
        Check whether the ThermalRadiationModel class could be set and get
        IIMPAR for advanced options
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setTemperatureListing(2)
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0.0</absorption_coefficient>
                    <directions_number>32</directions_number>
                    <temperature_listing_printing>2</temperature_listing_printing>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc),\
            'Could not set IIMPAR for advanced options in ThermalRadiationModel'
        assert mdl.getTemperatureListing() == 2,\
            'Could not get IIMPAR for advanced options in ThermalRadiationModel'

    def checkSetandGetTrs(self):
        """
        Check whether the ThermalRadiationModel class could be set and get
        IDIVER for advanced options
        """
        mdl = ThermalRadiationModel(self.case)
        mdl.setRadiativeModel('dom')
        mdl.setTrs(2)
        doc = '''<radiative_transfer model="dom">
                    <property label="Srad" name="srad"/>
                    <property label="Qrad" name="qrad"/>
                    <property label="Absorp" name="absorp"/>
                    <property label="Emiss" name="emiss"/>
                    <property label="CoefAb" name="coefAb"/>
                    <property label="Wall_temp" name="wall_temp" support="boundary"/>
                    <property label="Flux_incident" name="flux_incident" support="boundary"/>
                    <property label="Th_conductivity" name="thermal_conductivity" support="boundary"/>
                    <property label="Thickness" name="thickness" support="boundary"/>
                    <property label="Emissivity" name="emissivity" support="boundary"/>
                    <property label="Flux_net" name="flux_net" support="boundary"/>
                    <property label="Flux_convectif" name="flux_convectif" support="boundary"/>
                    <property label="Coeff_ech_conv" name="coeff_ech_conv" support="boundary"/>
                    <restart status="off"/>
                    <absorption_coefficient type="constant">0.0</absorption_coefficient>
                    <directions_number>32</directions_number>
                    <thermal_radiative_source_term>2</thermal_radiative_source_term>
                 </radiative_transfer>'''
        assert mdl.node_ray == self.xmlNodeFromString(doc),\
            'Could not set IDIVER for advanced options in ThermalRadiationModel'
        assert mdl.getTrs() == 2,\
            'Could not get IDIVER for advanced options in ThermalRadiationModel'

    def checkGetThermalRadiativeModel(self):
        """
        Check whether a thermal radiative model could be get
        """
        mdl = ThermalRadiationModel(self.case)
        assert mdl.getThermalRadiativeModel() == 1,\
            'Could not get thermal radiative model'



def suite():
    testSuite = unittest.makeSuite(ThermalRadiationTestCase, "check")
    return testSuite

def runTest():
    print("ThermalRadiationTestCase")
    runner = unittest.TextTestRunner()
    runner.run(suite())


#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
