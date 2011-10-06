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
This module defines the 'Additional user's scalars' page.

This module defines the following classes:
- DefineUserScalarsModel
"""

#-------------------------------------------------------------------------------
# Library modules import
#-------------------------------------------------------------------------------

import string
import unittest

#-------------------------------------------------------------------------------
# Application modules import
#-------------------------------------------------------------------------------

from Base.Common import *
import Base.Toolbox as Tool
from Base.XMLvariables import Variables
from Base.XMLvariables import Model
from Base.XMLmodel import XMLmodel, ModelTest
##from DefineBoundaryRegionsModel import DefBCModel

#-------------------------------------------------------------------------------
# Define User Scalars model class
#-------------------------------------------------------------------------------

class DefineUserScalarsModel(Variables, Model):
    """
    Useful methods for operation of the page.
    __method : private methods for the Model class.
    _method  : private methods for the View Class
    """
    def __init__(self, case):
        """
        Constructor
        """
        self.case = case

#        self.node_th_sca = self.case.xmlGetNode('thermal_scalar')
        self.scalar_node = self.case.xmlGetNode('additional_scalars')
        self.node_bc     = self.case.xmlGetNode('boundary_conditions')


    def defaultScalarValues(self):
        """Return the default values - Method also used by ThermalScalarModel"""
        default = {}
        default['scalar_label']          = "scalar"
        default['coefficient_label']     = "Dscal"
        default['initial_value']         = 0.0
        default['min_value']             = -1e+12
        default['max_value']             = 1e+12
        default['diffusion_coefficient'] = 1.83e-05
        default['diffusion_choice']      = 'constant'
        default['temperature_celsius']   = 20.0
        default['temperature_kelvin']    = 293.15
        default['enthalpy']              = 297413.
        default['zone_id']               = 1

        return default


    def __removeScalarChildNode(self, label, tag):
        """
        Private method.
        Delete 'variance' or 'property' markup from scalar named I{label}
        """
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            if node['label'] == label:
                node.xmlRemoveChild(tag)


    def __deleteScalarBoundaryConditions(self, label):
        """
        Private method.
        Delete boundary conditions for scalar I{label}
        """
        for nature in ('inlet', 'outlet', 'wall'):
            for node in self.node_bc.xmlGetChildNodeList(nature):
                for n in node.xmlGetChildNodeList('scalar'):
                    if n['label'] == label:
                        n.xmlRemoveNode()


    def __defaultScalarNameAndDiffusivityLabel(self, scalar_label=None):
        """
        Private method.
        Return a default name and label for a new scalar.
        Create a default name for the associated diffusion coefficient to.
        """
        __coef = {}
        for l in self.getScalarLabelsList():
            __coef[l] = self.getScalarDiffusivityLabel(l)
        length = len(__coef)
        Lscal = self.defaultScalarValues()['scalar_label']
        Dscal = self.defaultScalarValues()['coefficient_label']

        # new scalar: default value for both scalar and diffusivity

        if not scalar_label:
            if length != 0:
                i = 1
                while (Dscal + str(i)) in __coef.values():
                    i = i + 1
                num = str(i)
            else:
                num = str(1)
            scalar_label = Lscal + num
            __coef[scalar_label] = Dscal + num

        # existing scalar

        else:
            if scalar_label not in __coef.keys()or \
               (scalar_label in __coef.keys() and __coef[scalar_label] == ''):

                __coef[scalar_label] = Dscal + str(length + 1)

        return scalar_label, __coef[scalar_label]


    def __updateScalarNameAndDiffusivityName(self):
        """
        Private method.
        Update suffixe number for scalar name and diffusivity' name.
        """
        list = []
        n = 0
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            n = n + 1
            if node['type'] == 'user':
                node['name'] = 'scalar' + str(n)
            nprop = node.xmlGetChildNode('property')
            if nprop:
                nprop['name'] = 'diffusion_coefficient_' + str(n)


    def __setScalarDiffusivity(self, scalar_label, coeff_label):
        """
        Private method.

        Input default initial value of property "diffusivity"
        for a new scalar I{scalar_label}
        """
        self.isNotInList(scalar_label, self.getScalarsVarianceList())
        self.isInList(scalar_label, self.getUserScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', type='user', label=scalar_label)
        n.xmlInitChildNode('property', label=coeff_label)

        if not self.getScalarDiffusivityChoice(scalar_label):
            self.setScalarDiffusivityChoice(scalar_label, 'constant')

        if not self.getScalarDiffusivityInitialValue(scalar_label):
            ini = self.defaultScalarValues()['diffusion_coefficient']
            self.setScalarDiffusivityInitialValue(scalar_label, ini)


    def __deleteScalar(self, label):
        """
        Private method.

        Delete scalar I{label}.
        """
        node = self.scalar_node.xmlGetNode('scalar', label=label)
        node.xmlRemoveNode()
        self.__deleteScalarBoundaryConditions(label)
        self.__updateScalarNameAndDiffusivityName()


    def getScalarLabelsList(self):
        """Public method.
        Return the User scalar label list (thermal scalar included)"""
        list = []
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            list.append(node['label'])
        return list


    def getUserScalarLabelsList(self):
        """Public method.
        Return the user scalar label list (without thermal scalar).
        Method also used by UserScalarPropertiesView
        """
        list = []
        for node in self.scalar_node.xmlGetNodeList('scalar', type='user'):
            list.append(node['label'])
        return list


    def setScalarBoundaries(self):
        """Public method.
        Input boundaries conditions for a scalar node. Method also used by ThermalScalarModel
        """
        from Pages.Boundary import Boundary

        for node in self.node_bc.xmlGetChildNodeList('inlet'):
            model = Boundary('inlet', node['label'], self.case)
            for label in self.getScalarLabelsList():
                model.setScalar(label, 0.0)

        for node in self.node_bc.xmlGetChildNodeList('outlet'):
            model = Boundary('outlet', node['label'], self.case)
            for label in self.getScalarLabelsList():
                model.setScalar(label, 0.0)


    def addUserScalar(self, zone, label=None):
        """Public method.
        Input a new user scalar I{label}"""
        self.isInt(int(zone))

        l, c = self.__defaultScalarNameAndDiffusivityLabel(label)

        if l not in self.getScalarLabelsList():
            self.scalar_node.xmlInitNode('scalar', 'name', type="user", label=l)

            ini = self.defaultScalarValues()['initial_value']
            min = self.defaultScalarValues()['min_value']
            max = self.defaultScalarValues()['max_value']

            self.setScalarInitialValue(zone, l, ini)
            self.setScalarMinValue(l, min)
            self.setScalarMaxValue(l, max)
            self.__setScalarDiffusivity(l, c)
            self.setScalarBoundaries()

        self.__updateScalarNameAndDiffusivityName()

        return l


    def renameScalarLabel(self, old_label, new_label):
        """Public method.
        Modify old_label of scalar with new_label and put new label if variancy exists"""
        # fusion de cette methode avec OutputVolumicVariablesModel.setVariablesLabel
        self.isInList(old_label, self.getScalarLabelsList())

        label = new_label[:LABEL_LENGTH_MAX]
        if label not in self.getScalarLabelsList():
            for node in self.scalar_node.xmlGetNodeList('scalar'):
                if node['label'] == old_label:
                    node['label'] = label

                if node.xmlGetString('variance') == old_label:
                    node.xmlSetData('variance', label)

        for nature in ('inlet', 'outlet', 'wall'):
            for node in self.node_bc.xmlGetChildNodeList(nature):
                for n in node.xmlGetChildNodeList('scalar'):
                    if n['label'] == old_label:
                        n['label'] = new_label

        for node in self.case.xmlGetNodeList('formula'):
            f = node.xmlGetTextNode().replace(old_label, new_label)
            node.xmlSetTextNode(f)


    # FIXME: cette methode est a deplacer dans ThermalScalarmodel
    def getThermalScalarLabel(self):
        """
        Get label for thermal scalar
        """
        label = ""
        node = self.scalar_node.xmlGetNode('scalar', type='thermal')
        if node:
            label = node['label']

        return label


    def getScalarInitialValue(self, zone, scalar_label):
        """
        Get initial value from an additional_scalar with label scalar_label
        and zone zone. Method also used by InitializationView
        """
        self.isInt(int(zone))
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        val = n.xmlGetChildDouble('initial_value', zone_id=zone)
        if val == None:
            if n['type'] == 'thermal':
                val = self.defaultScalarValues()[n['name']]
            else:
                val = self.defaultScalarValues()['initial_value']
            self.setScalarInitialValue(zone, scalar_label, val)

        return val


    def setScalarInitialValue(self, zone, scalar_label, initial_value):
        """
        Put initial value for an additional_scalar with label scalar_label
        and zone zone.
        Method also used by InitializationView, ThermalScalarModel
        """
        self.isInt(int(zone))
        self.isFloat(initial_value)
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        nz = n.xmlInitChildNode('initial_value', zone_id=zone)
        nz.xmlSetTextNode(initial_value)


    def getScalarMinValue(self, scalar_label):
        """Get minimal value from an additional_scalar with label scalar_label"""
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        min_val = n.xmlGetChildDouble('min_value')
        if min_val == None:
            min_val = self.defaultScalarValues()['min_value']
            if self.getScalarVariance(scalar_label) == "":
                self.setScalarMinValue(scalar_label, min_val)

        return min_val


    def setScalarMinValue(self, scalar_label, min_value):
        """
        Put minimal value for an additional_scalar with label scalar_label.
        Method also used by ThermalScalarModel
        """
        self.isFloat(min_value)
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        n.xmlSetData('min_value', min_value)


    def getScalarMaxValue(self, scalar_label):
        """Get maximal value from an additional_scalar with label scalar_label"""
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        max_val = n.xmlGetDouble('max_value')
        if max_val == None:
            max_val = self.defaultScalarValues()['max_value']
            if self.getScalarVariance(scalar_label) == "":
                self.setScalarMaxValue(scalar_label, max_val)

        return max_val


    def setScalarMaxValue(self, scalar_label, max_value):
        """
        Put maximal value for an additional_scalar with label scalar_label.
        Method also used by ThermalScalarModel
        """
        # we verify max_value is a float value
        self.isFloat(max_value)
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        n.xmlSetData('max_value', max_value)


    def getScalarVariance(self, l):
        """
        Get variance of an additional_scalar with label I{l}.
        Method also used by UserScalarPropertiesView
        """
        self.isInList(l, self.getScalarLabelsList())

        return self.scalar_node.xmlGetNode('scalar', label=l).xmlGetString('variance')


    def setScalarVariance(self, scalar_label, variance_label):
        """Put variance of an additional_scalar with label scalar_label"""
        self.isInList(scalar_label, self.getUserScalarLabelsList())
        self.isInList(variance_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', type='user', label=scalar_label)
        n.xmlSetData('variance', variance_label)

        self.__removeScalarChildNode(scalar_label, 'property')
        self.setScalarMinValue(scalar_label, 0.0)
        self.setScalarMaxValue(scalar_label, self.defaultScalarValues()['max_value'])


    def getScalarsWithVarianceList(self):
        """
        Return list of scalars which have a variance
        """
        list = []
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            sca = node.xmlGetString('variance')
            if sca and sca not in list:
                list.append(sca)
        return list


    def getScalarsVarianceList(self):
        """
        Return list of scalars which are also a variance
        """
        list = []
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            if node.xmlGetString('variance') and node['label'] not in list:
                list.append(node['label'])
        return list


    def getVarianceLabelFromScalarLabel(self, label):
        """
        Get the label of scalar with variancy's label: label
        """
        self.isInList(label, self.getScalarLabelsList())

        lab = ""
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            if node.xmlGetString('variance') == label:
                lab = node['label']
        return lab


    def setScalarDiffusivityLabel(self, scalar_label, diff_label):
        """
        Set label of diffusivity's property for an additional_scalar
        """
        self.isInList(scalar_label, self.getScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        n.xmlGetChildNode('property')['label'] = diff_label


    def getScalarDiffusivityLabel(self, scalar_label):
        """
        Get label of diffusivity's property for an additional_scalar
        with label scalar_label
        """
        self.isInList(scalar_label, self.getScalarLabelsList())

        lab_diff = ""
        n = self.scalar_node.xmlGetNode('scalar', label=scalar_label)
        n_diff = n.xmlGetChildNode('property')
        if n_diff:
            lab_diff = n_diff['label']

        return lab_diff


    def setScalarDiffusivityInitialValue(self, scalar_label, initial_value):
        """
        Set initial value of diffusivity's property for an additional_scalar
        with label scalar_label. Method also called by UserScalarPropertiesView.
        """
        self.isNotInList(scalar_label, self.getScalarsVarianceList())
        self.isInList(scalar_label, self.getUserScalarLabelsList())
        self.isFloat(initial_value)

        n = self.scalar_node.xmlGetNode('scalar', type='user', label=scalar_label)
        n_diff = n.xmlInitChildNode('property')
        n_diff.xmlSetData('initial_value', initial_value)


    def getScalarDiffusivityInitialValue(self, scalar_label):
        """
        Get initial value of diffusivity's property for an additional_scalar
        with label scalar_label. Method also called by UserScalarPropertiesView.
        """
        self.isNotInList(scalar_label, self.getScalarsVarianceList())
        self.isInList(scalar_label, self.getUserScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', type='user', label=scalar_label)
        n_diff = n.xmlInitChildNode('property')
        diffu = n_diff.xmlGetDouble('initial_value')
        if diffu == None:
            diffu = self.defaultScalarValues()['diffusion_coefficient']
            self.setScalarDiffusivityInitialValue(scalar_label, diffu)

        return diffu


    def setScalarDiffusivityChoice(self, scalar_label, choice):
        """
        Set choice of diffusivity's property for an additional_scalar
        with label scalar_label
        """
        self.isNotInList(scalar_label, self.getScalarsVarianceList())
        self.isInList(scalar_label, self.getUserScalarLabelsList())
        self.isInList(choice, ('constant', 'variable'))

        n = self.scalar_node.xmlGetNode('scalar', type='user', label=scalar_label)
        n_diff = n.xmlInitChildNode('property')
        n_diff['choice'] = choice


    def getScalarDiffusivityChoice(self, scalar_label):
        """
        Get choice of diffusivity's property for an additional_scalar
        with label scalar_label
        """
        self.isNotInList(scalar_label, self.getScalarsVarianceList())
        self.isInList(scalar_label, self.getUserScalarLabelsList())

        n = self.scalar_node.xmlGetNode('scalar', type='user', label=scalar_label)
        choice = n.xmlInitChildNode('property')['choice']
        if not choice:
            choice = self.defaultScalarValues()['diffusion_choice']
            self.setScalarDiffusivityChoice(scalar_label, choice)

        return choice


    def setScalarValues(self, label, zone, init, mini, maxi, vari):
        """
        Put values to scalar with labelled I{label} for creating or replacing values.
        """
        self.isInt(int(zone))
        self.isFloat(init)
        self.isFloat(mini)
        self.isFloat(maxi)
        l = self.getScalarLabelsList()
        l.append('no')
        self.isInList(vari, l)

        type = 'user'

        if label not in self.getUserScalarLabelsList():
            if label not in self.getScalarLabelsList():
                self.addUserScalar(zone, label)
            else:
                type = 'thermal'
                self.setScalarMinValue(label, mini)
                self.setScalarMaxValue(label, maxi)

        self.setScalarInitialValue(zone, label, init)
        if type == 'user':
            if vari != "no":
                self.setScalarVariance(label, vari)
            else:
                self.__removeScalarChildNode(label, 'variance')
                self.setScalarMinValue(label, mini)
                self.setScalarMaxValue(label, maxi)
                l, c = self.__defaultScalarNameAndDiffusivityLabel(label)
                self.__setScalarDiffusivity(l, c)

        self.__updateScalarNameAndDiffusivityName()


    def deleteScalar(self, slabel):
        """
        Public method.
        Delete scalar I{label}. Also called by ThermalScalarModel
        Warning: deleting a scalar may delete other scalar which are variances
        of previous deleting scalars.
        """
        self.isInList(slabel, self.getScalarLabelsList())

        # First add the main scalar to delete
        list = []
        list.append(slabel)

        # Then add variance scalar related to the main scalar
        for node in self.scalar_node.xmlGetNodeList('scalar'):
            if node.xmlGetString('variance') == slabel:
                list.append(node['label'])

        # Delete all scalars
        for scalar in list:
            self.__deleteScalar(scalar)

        return list


    def getScalarType(self, scalar_label):
        """
        Return type of scalar for choice of color (for view)
        """
        self.isInList(scalar_label, self.getScalarLabelsList())
        node = self.scalar_node.xmlGetNode('scalar', 'type', label=scalar_label)
        Model().isInList(node['type'], ('user', 'thermal'))
        return node['type']


    def getScalarName(self, scalar_label):
        """
        Return type of scalar for choice of color (for view)
        """
        self.isInList(scalar_label, self.getScalarLabelsList())
        node = self.scalar_node.xmlGetNode('scalar', 'name', label=scalar_label)
        return node['name']


#-------------------------------------------------------------------------------
# DefineUsersScalars test case
#-------------------------------------------------------------------------------


class UserScalarTestCase(ModelTest):
    """
    Unittest.
    """
    def checkDefineUserScalarsModelInstantiation(self):
        """Check whether the DefineUserScalarsModel class could be instantiated."""
        model = None
        model = DefineUserScalarsModel(self.case)

        assert model != None, 'Could not instantiate DefineUserScalarsModel'


    def checkAddNewUserScalar(self):
        """Check whether the DefineUserScalarsModel class could add a scalar."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'toto')

        doc = '''<additional_scalars>
                    <scalar label="toto" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0 </initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
           'Could not add a user scalar'

    def checkRenameScalarLabel(self):
        """Check whether the DefineUserScalarsModel class could set a label."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'toto')
        model.addUserScalar(zone,'titi')
        model.renameScalarLabel('titi', 'MACHIN')

        doc = '''<additional_scalars>
                    <scalar label="toto" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="MACHIN" name="scalar2" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal2" name="diffusion_coefficient_2">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
           'Could not rename a label to one scalar'

    def checkGetThermalScalarLabel(self):
        """Check whether the DefineUserScalarsModel class could be get label of thermal scalar."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'usersca1')
        from Pages.ThermalScalarModel import ThermalScalarModel
        ThermalScalarModel(self.case).setThermalModel('temperature_celsius')
        del ThermalScalarModel

        doc = '''<additional_scalars>
                    <scalar label="usersca1" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="TempC" name="temperature_celsius" type="thermal">
                            <initial_value zone_id="1">20.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                    </scalar>
                </additional_scalars>'''

        model.renameScalarLabel("TempC", "Matemperature")

        assert model.getThermalScalarLabel() == "Matemperature",\
           'Could not get label of thermal scalar'

    def checkSetAndGetScalarInitialValue(self):
        """Check whether the DefineUserScalarsModel class could be set and get initial value."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'toto')
        model.setScalarInitialValue(zone, 'toto', 0.05)

        doc = '''<additional_scalars>
                    <scalar label="toto" name="scalar1" type="user">
                            <initial_value zone_id="1">0.05</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
            'Could not set initial value to user scalar'
        assert model.getScalarInitialValue(zone, 'toto') == 0.05,\
            'Could not get initial value to user scalar'

    def checkSetAndGetScalarMinMaxValue(self):
        """Check whether the DefineUserScalarsModel class could be set and get min and max value."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'toto')
        model.addUserScalar(zone, 'titi')
        model.setScalarInitialValue(zone, 'toto', 0.05)
        model.setScalarMinValue('toto',0.01)
        model.setScalarMaxValue('titi',100.)

        doc = '''<additional_scalars>
                    <scalar label="toto" name="scalar1" type="user">
                            <initial_value zone_id="1">0.05</initial_value>
                            <min_value>0.01</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="titi" name="scalar2" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>100.</max_value>
                            <property choice="constant" label="Dscal2" name="diffusion_coefficient_2">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
            'Could not set minimal or maximal value to user scalar'
        assert model.getScalarMinValue('toto') == 0.01,\
            'Could not get minimal value from user scalar'
        assert model.getScalarMaxValue('titi') == 100.,\
            'Could not get maximal value from user scalar'

    def checkSetAndGetScalarVariance(self):
        """Check whether the DefineUserScalarsModel class could be set and get variance of scalar."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'toto')
        model.addUserScalar(zone, 'titi')
        model.setScalarVariance('toto', 'titi')

        doc = '''<additional_scalars>
                    <scalar label="toto" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>0</min_value>
                            <max_value>1e+12</max_value>
                            <variance>titi</variance>
                    </scalar>
                    <scalar label="titi" name="scalar2" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal2" name="diffusion_coefficient_2">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
            'Could not set variance to user scalar'
        assert model.getScalarVariance('toto') == 'titi',\
            'Could not get variance of user scalar'

    def checkGetVarianceLabelFromScalarLabel(self):
        """
        Check whether the DefineUserScalarsModel class could be get label of
        the scalar which has variancy.
        """
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'toto')
        model.addUserScalar(zone, 'titi')
        model.setScalarVariance('toto', 'titi')

        assert model.getVarianceLabelFromScalarLabel('titi') == 'toto',\
            'Could not get label of scalar whiwh has a variancy'

    def checkGetScalarDiffusivityLabel(self):
        """
        Check whether the DefineUserScalarsModel class could be get label of
        diffusivity of user scalar.
        """
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'premier')
        model.addUserScalar(zone, 'second')

        doc = '''<additional_scalars>
                    <scalar label="premier" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>0</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="second" name="scalar2" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal2" name="diffusion_coefficient_2">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.getScalarDiffusivityLabel('second') == "Dscal2",\
            'Could not get label of diffusivity of one scalar'

    def checkSetandGetScalarDiffusivityInitialValue(self):
        """
        Check whether the DefineUserScalarsModel class could be set
        and get initial value of diffusivity.
        """
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'premier')
        model.addUserScalar(zone, 'second')
        model.setScalarDiffusivityInitialValue('premier', 0.555)

        doc = '''<additional_scalars>
                    <scalar label="premier" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>0.555</initial_value>
                            </property>
                    </scalar>
                    <scalar label="second" name="scalar2" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal2" name="diffusion_coefficient_2">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
            'Could not set initial value of property of one user scalar'
        assert model.getScalarDiffusivityInitialValue('premier') == 0.555,\
            'Could not get initial value of property of one user scalar '

    def checkSetandGetScalarDiffusivityChoice(self):
        """Check whether the DefineUserScalarsModel class could be set and get diffusivity's choice."""
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'premier')
        model.addUserScalar(zone, 'second')
        model.setScalarDiffusivityChoice('premier', 'variable')
        doc = '''<additional_scalars>
                    <scalar label="premier" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="variable" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="second" name="scalar2" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal2" name="diffusion_coefficient_2">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
            'Could not set choice of property of one user scalar'
        assert model.getScalarDiffusivityChoice('premier') == "variable",\
            'Could not get choice of property of one user scalar'

    def checkDeleteScalarandGetScalarType(self):
        """
        Check whether the DefineUserScalarsModel class could be
        delete a user scalar and get type of the scalar.
        """
        model = DefineUserScalarsModel(self.case)
        zone = '1'
        model.addUserScalar(zone, 'premier')

        from Pages.ThermalScalarModel import ThermalScalarModel
        ThermalScalarModel(self.case).setThermalModel('temperature_celsius')
        del ThermalScalarModel

        model.addUserScalar(zone, 'second')
        model.addUserScalar(zone, 'troisieme')
        model.addUserScalar(zone, 'quatrieme')

        assert model.getScalarType('premier') == 'user',\
            'Could not get type of one scalar'

        model.deleteScalar('second')

        doc = '''<additional_scalars>
                    <scalar label="premier" name="scalar1" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal1" name="diffusion_coefficient_1">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="TempC" name="temperature_celsius" type="thermal">
                            <initial_value zone_id="1">20.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                    </scalar>
                    <scalar label="troisieme" name="scalar3" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal4" name="diffusion_coefficient_3">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                    <scalar label="quatrieme" name="scalar4" type="user">
                            <initial_value zone_id="1">0.0</initial_value>
                            <min_value>-1e+12</min_value>
                            <max_value>1e+12</max_value>
                            <property choice="constant" label="Dscal5" name="diffusion_coefficient_4">
                                    <initial_value>1.83e-05</initial_value>
                            </property>
                    </scalar>
                </additional_scalars>'''

        assert model.scalar_node == self.xmlNodeFromString(doc),\
            'Could not delete one scalar'



def suite():
    """unittest function"""
    testSuite = unittest.makeSuite(UserScalarTestCase, "check")
    return testSuite


def runTest():
    """unittest function"""
    print("UserScalarTestTestCase")
    runner = unittest.TextTestRunner()
    runner.run(suite())


#-------------------------------------------------------------------------------
# End DefineUsersScalars
#-------------------------------------------------------------------------------
