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
This module contains the following classes and function:
- OutputVariableModel
- OutputVariableModelTestCase
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import string, unittest

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Common import *
import Base.Toolbox as Tool
from Base.XMLmodel import ModelTest
from Base.XMLvariables import Model
from Pages.ThermalScalarModel import ThermalScalarModel
from Pages.ThermalRadiationModel import ThermalRadiationModel

#-------------------------------------------------------------------------------
# Output model class
#-------------------------------------------------------------------------------

class OutputSurfacicVariablesModel(Model):

    def __init__(self, case):
        """
        Constuctor
        """
        self.case = case
        self.node_models = self.case.xmlGetNode('thermophysical_models')
        self.listNodeSurface = (self._getListOfVelocityPressureSurfacicProperties(),
                                self._getThermalScalarSurfacicProperties(),
                                self._getThermalRadiativeSurfacicProperties())

        self.dicoLabelName = {}
        self.list_name = []
        self._updateDicoLabelName()


    def _defaultValues(self):
        """
        Return in a dictionnary which contains default values
        """
        default = {}
        default['status']    = "on"

        return default


    def _getListOfVelocityPressureSurfacicProperties(self):
        """
        Private method: return node of yplus, all_variables and effort
        """
        nodeList = []
        self.node_veloce = self.node_models.xmlInitNode('velocity_pressure')
        for node in self.node_veloce.xmlGetNodeList('property'):
            if node['support']:
                nodeList.append(node)
        return nodeList


    def _getThermalScalarSurfacicProperties(self):
        """
        Private method: return list of volumic properties for thermal radiation
        """
        nodeList = []
        self.node_therm = self.node_models.xmlGetNode('thermal_scalar')
        if ThermalScalarModel(self.case).getThermalScalarModel() != "off":
            for node in self.node_therm.xmlGetChildNodeList('property'):
                if node['support']:
                    nodeList.append(node)
        return nodeList


    def _getThermalRadiativeSurfacicProperties(self):
        """
        Private method: return list of volumic properties for thermal radiation
        """
        nodeList = []
        self.node_ray = self.node_models.xmlGetNode('radiative_transfer')
        if ThermalRadiationModel(self.case).getRadiativeModel() != "off":
            for node in self.node_ray.xmlGetChildNodeList('property'):
                if node['support']:
                    nodeList.append(node)
        return nodeList


    def _updateDicoLabelName(self):
        """
        Private method: update dictionaries of labels for all properties .....
        """
        for nodeList in self.listNodeSurface:
            for node in nodeList:
                name = node['name']
                if not name:
                    name = node['label']
                if not node['label']:
                    msg = "xml node named "+ name +" has no label"
                    raise ValueError(msg)
                self.dicoLabelName[name] = node['label']
                self.list_name.append(name)


    def getLabelsList(self):
        """
        Return list of labels for all properties .....Only for the View
        """
        list = []
        for nodeList in self.listNodeSurface:
            for node in nodeList:
                list.append(node['label'])
        return list


    def setPropertyLabel(self, old_label, new_label):
        """
        Replace old_label by new_label for node with name and old_label. Only for the View
        """
        self.isInList(old_label, self.getLabelsList())
        if old_label != new_label:
            self.isNotInList(new_label, self.getLabelsList())
        for nodeList in self.listNodeSurface:
            for node in nodeList:
                if node['label'] == old_label:
                    node['label'] = new_label
        self._updateDicoLabelName()


    def getPostProcessing(self, label):
        """ Return status of post processing for node withn label 'label'"""
        self.isInList(label, self.getLabelsList())
        status = self._defaultValues()['status']
        for nodeList in self.listNodeSurface:
            for node in nodeList:
                if node['label'] == label:
                    node_post = node.xmlGetChildNode('postprocessing_recording', 'status')
                    if node_post:
                        status = node_post['status']
        return status


    def setPostProcessing(self, label, status):
        """ Put status of post processing for node with label 'label'"""
        self.isOnOff(status)
        self.isInList(label, self.getLabelsList())
        for nodeList in self.listNodeSurface:
            for node in nodeList:
                if node['label'] == label:
                    if status == 'off':
                        node.xmlInitChildNode('postprocessing_recording')['status'] = status
                    else:
                        if node.xmlGetChildNode('postprocessing_recording'):
                            node.xmlRemoveChild('postprocessing_recording')

#-------------------------------------------------------------------------------
# OutputVariableModel test case
#-------------------------------------------------------------------------------

class OutputSurfacicVariablesTestCase(ModelTest):
    """
    """
    def checkOutputSurfacicVariablesInstantiation(self):
        """
        Check whether the OutputSurfacicVariables Model class could be instantiated
        """
        model = None
        model = OutputSurfacicVariablesModel(self.case)
        assert model != None, 'Could not instantiate OutputSurfacicVariablesModel'

    def checkSetPropertyLabel(self):
        """
        Check whether the OutputSurfacicVariablesModel class could be set a label
        of property
        """
        model = OutputSurfacicVariablesModel(self.case)
        model.setPropertyLabel('Yplus', 'toto')
        node = model.node_models.xmlInitNode('velocity_pressure')
        doc = '''<velocity_pressure>
                    <variable label="Pressure" name="pressure"/>
                    <variable label="VelocitU" name="velocity_U"/>
                    <variable label="VelocitV" name="velocity_V"/>
                    <variable label="VelocitW" name="velocity_W"/>
                    <property label="total_pressure" name="total_pressure"/>
                    <property label="toto" name="yplus" support="boundary"/>
                    <property label="Efforts" name="effort" support="boundary"/>
                    <property label="all_variables" name="all_variables" support="boundary"/>
                 </velocity_pressure>'''
        assert node == self.xmlNodeFromString(doc),\
            'Could not set label of property in output surfacic variables model'

    def checkSetandGetPostProcessing(self):
        """
        Check whether the OutputSurfacicVariablesModel class could be set and
        get status for post processing of named property
        """
        from Pages.ThermalRadiationModel import ThermalRadiationModel
        ThermalRadiationModel(self.case).setRadiativeModel('dom')
        del ThermalRadiationModel
        model = OutputSurfacicVariablesModel(self.case)

        model.setPostProcessing('Flux_convectif','off')
        doc = '''<property label="Flux_convectif" name="flux_convectif" support="boundary">
                    <postprocessing_recording status="off"/>
                 </property>'''

        assert model.getPostProcessing('Flux_convectif') == 'off',\
                'Could not get status for post processing of named property'


def suite():
    testSuite = unittest.makeSuite(OutputSurfacicVariablesTestCase, "check")
    return testSuite


def runTest():
    print("OutputSurfacicVariablesTestCase")
    runner = unittest.TextTestRunner()
    runner.run(suite())

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
