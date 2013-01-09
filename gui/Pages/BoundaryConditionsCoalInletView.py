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
- BoundaryConditionsCoalInletView
- ValueDelegate
- StandardItemModelCoal
- StandardItemModelCoalMass
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

from Pages.BoundaryConditionsCoalInletForm import Ui_BoundaryConditionsCoalInletForm
import Pages.CoalCombustionModel as CoalCombustion

from Base.Toolbox import GuiParam
from Base.QtPage import DoubleValidator, ComboModel, setGreenColor
from Pages.LocalizationModel import LocalizationModel, Zone
from Pages.Boundary import Boundary

from Pages.QMeiEditorView import QMeiEditorView

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("BoundaryConditionsCoalInletView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# Line edit delegate with a Double validator (positive value)
#-------------------------------------------------------------------------------

class ValueDelegate(QItemDelegate):
    def __init__(self, parent=None):
        super(ValueDelegate, self).__init__(parent)
        self.parent = parent

    def createEditor(self, parent, option, index):
        editor = QLineEdit(parent)
        validator = DoubleValidator(editor, min=0.)
        editor.setValidator(validator)
        #editor.installEventFilter(self)
        return editor

    def setEditorData(self, editor, index):
        value = index.model().data(index, Qt.DisplayRole).toString()
        editor.setText(value)

    def setModelData(self, editor, model, index):
        value, ok = editor.text().toDouble()
        if editor.validator().state == QValidator.Acceptable:
            model.setData(index, QVariant(value), Qt.DisplayRole)

#-------------------------------------------------------------------------------
# StandarItemModel class to display Coals in a QTableView
#-------------------------------------------------------------------------------

class StandardItemModelCoal(QStandardItemModel):
    def __init__(self, case):
        QStandardItemModel.__init__(self)
        self.headers = [self.tr("Coal number"),
                        self.tr("Flow (kg/s)"),
                        self.tr("Temperature \n(K)")]
        self.setColumnCount(len(self.headers))
        self.dataCoal = []
        self.__case = case


    def setBoundaryFromLabel(self, label):
        self.modelBoundary = Boundary('coal_inlet', label, self.__case)


    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            return QVariant(self.dataCoal[index.row()][index.column()])
        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.column() in [1,2]:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        row = index.row()
        col = index.column()
        if not hasattr(self, "modelBoundary"):
            log.debug("ERROR in setData (StandardItemModelCoal) : no Boundary model defined")
            return
        v, ok = value.toDouble()
        self.dataCoal[row][col] = v
        if col == 1:
            self.modelBoundary.setCoalFlow(v, row)
        elif col == 2:
            self.modelBoundary.setCoalTemperature(v, row)
        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def insertItem(self, nameCoal, valCoal, valCoalTemp):
        line = [nameCoal, valCoal, valCoalTemp]
        self.dataCoal.append(line)
        row = self.rowCount()
        self.setRowCount(row+1)


    def deleteAll(self):
        self.dataCoal = []
        self.setRowCount(0)

#-------------------------------------------------------------------------------
# StandarItemModel class to display Coal masses in a QTableView
#-------------------------------------------------------------------------------

class StandardItemModelCoalMass(QStandardItemModel):

    def __init__(self, case, coalNumber, coalClassesNumber):
        QStandardItemModel.__init__(self)
        self.__case = case
        self.coalNumber = coalNumber
        self.coalClassesNumber = coalClassesNumber


    def setRatio(self, ratio):
        cols = len(ratio)
        if type(ratio[0]) == type([]):
            rows = max([len(c) for c in ratio])
        else:
            rows = 1
        self.setColumnCount(cols)
        self.setRowCount(rows)
        self.ratio = ratio


    def setBoundaryFromLabel(self, label):
        log.debug("setBoundaryFromLabel")
        self.modelBoundary = Boundary('coal_inlet', label, self.__case)


    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            classe = index.row()
            coal   = index.column()
            if classe < self.coalClassesNumber[coal]:
                try:
                    return QVariant(self.ratio[coal][classe])
                except:
                    log.debug("ERROR no data for self.ratio[%i][%i] "%(coal, classe))
        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.row() >= self.coalClassesNumber[index.column()]:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant("Coal" + " " + str(section+1))
        if orientation == Qt.Vertical and role == Qt.DisplayRole:
            return QVariant("Class" + " " + str(section+1))
        return QVariant()


    def setData(self, index, value, role):
        if not hasattr(self, "modelBoundary"):
            log.debug("ERROR in setData (StandardItemModelCoalMass): no Boundary model defined")
            return
        classe = index.row()
        coal   = index.column()
        v, ok = value.toDouble()
        self.ratio[coal][classe] = v
        log.debug("setData v = %f "%v)

        liste = self.modelBoundary.getCoalRatios(coal)
        lastValue = 0
        for iclasse in range(0, self.coalClassesNumber[coal]-1):
            lastValue += self.ratio[coal][iclasse]

        if lastValue < 100.+ 1e-6 :
            liste[classe] = self.ratio[coal][classe]
            lastValue = 100 - lastValue
            self.ratio[coal][self.coalClassesNumber[coal]-1] = lastValue
            liste[self.coalClassesNumber[coal]-1] = lastValue
            self.modelBoundary.setCoalRatios(coal, liste)
        else :
            self.ratio[coal][classe] = liste[classe]

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def deleteAll(self):
        self.ratio = []
        self.setRowCount(0)

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class BoundaryConditionsCoalInletView(QWidget, Ui_BoundaryConditionsCoalInletForm):
    def __init__(self, parent):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_BoundaryConditionsCoalInletForm.__init__(self)
        self.setupUi(self)


    def setup(self, case):
        """
        Setup the widget
        """
        self.__case = case
        self.__boundary = None

        self.__case.undoStopGlobal()

        # Connections
        self.connect(self.comboBoxTypeInlet,
                     SIGNAL("activated(const QString&)"),
                     self.__slotInletType)
        self.connect(self.comboBoxVelocity,
                     SIGNAL("activated(const QString&)"),
                     self.__slotChoiceVelocity)
        self.connect(self.lineEditVelocity,
                     SIGNAL("textChanged(const QString &)"),
                     self.__slotVelocityValue)
        self.connect(self.lineEditTemperature,
                     SIGNAL("textChanged(const QString &)"),
                     self.__slotTemperature)
        self.connect(self.spinBoxOxydantNumber,
                     SIGNAL("valueChanged(int)"),
                     self.__slotOxydantNumber)

        self.connect(self.comboBoxDirection,
                     SIGNAL("activated(const QString&)"),
                     self.__slotChoiceDirection)
        self.connect(self.lineEditDirectionX,
                     SIGNAL("textChanged(const QString &)"),
                     self.__slotDirX)
        self.connect(self.lineEditDirectionY,
                     SIGNAL("textChanged(const QString &)"),
                     self.__slotDirY)
        self.connect(self.lineEditDirectionZ,
                     SIGNAL("textChanged(const QString &)"),
                     self.__slotDirZ)

        # Combo models
        self.modelTypeInlet = ComboModel(self.comboBoxTypeInlet, 2, 1)
        self.modelTypeInlet.addItem(self.tr("Only oxydant"), 'oxydantFlow')
        self.modelTypeInlet.addItem(self.tr("Oxydant and coal"), 'coalFlow')

        self.modelVelocity = ComboModel(self.comboBoxVelocity, 4, 1)
        self.modelVelocity.addItem(self.tr("Norm"), 'norm')
        self.modelVelocity.addItem(self.tr("Mass flow rate"), 'flow1')
        self.modelVelocity.addItem(self.tr("Norm (user law)"), 'norm_formula')
        self.modelVelocity.addItem(self.tr("Mass flow rate (user law)"), 'flow1_formula')

        self.modelDirection = ComboModel(self.comboBoxDirection, 3, 1)
        self.modelDirection.addItem(self.tr("Normal to the inlet"), 'normal')
        self.modelDirection.addItem(self.tr("Specified coordinates"), 'coordinates')
        self.modelDirection.addItem(self.tr("User profile"), 'formula')

        # Validators
        validatorVelocity = DoubleValidator(self.lineEditVelocity)
        validatorX = DoubleValidator(self.lineEditDirectionX)
        validatorY = DoubleValidator(self.lineEditDirectionY)
        validatorZ = DoubleValidator(self.lineEditDirectionZ)
        validatorTemp = DoubleValidator(self.lineEditTemperature, min=0.)

        # Apply validators
        self.lineEditVelocity.setValidator(validatorVelocity)
        self.lineEditDirectionX.setValidator(validatorX)
        self.lineEditDirectionY.setValidator(validatorY)
        self.lineEditDirectionZ.setValidator(validatorZ)
        self.lineEditTemperature.setValidator(validatorTemp)

        self.connect(self.pushButtonVelocityFormula,
                     SIGNAL("clicked()"),
                     self.__slotVelocityFormula)
        self.connect(self.pushButtonDirectionFormula,
                     SIGNAL("clicked()"),
                     self.__slotDirectionFormula)

        # Useful information about coals, classes, and ratios

        mdl =  CoalCombustion.CoalCombustionModel(self.__case)
        if mdl.getCoalCombustionModel() != "off":
            self.__coalNumber = mdl.getCoalNumber()
            self.__coalClassesNumber = []
            for coal in range(0, self.__coalNumber):
                self.__coalClassesNumber.append(mdl.getClassNumber(str(coal+1)))
            self.__maxOxydantNumber = mdl.getOxidantNumber()
        else:
            self.__coalNumber = 0
            self.__coalClassesNumber = [0]
            self.__maxOxydantNumber = 1

        self.__ratio = self.__coalNumber*[0]
        for i in range(0, self.__coalNumber):
            self.__ratio[i] = self.__coalClassesNumber[i]*[0]

        # Coal table

        self.__modelCoal = StandardItemModelCoal(self.__case)
        self.tableViewCoal.setModel(self.__modelCoal)
        delegateValue = ValueDelegate(self.tableViewCoal)
        self.tableViewCoal.setItemDelegateForColumn(1, delegateValue)
        self.tableViewCoal.setItemDelegateForColumn(2, delegateValue)

        # Coal mass ratio table

        self.__modelCoalMass = StandardItemModelCoalMass(self.__case,
                                                         self.__coalNumber,
                                                         self.__coalClassesNumber)
        self.tableViewCoalMass.setModel(self.__modelCoalMass)

        delegateValueMass = ValueDelegate(self.tableViewCoalMass)
        for c in range(self.__modelCoalMass.columnCount()):
            self.tableViewCoalMass.setItemDelegateForColumn(c, delegateValueMass)

        self.__case.undoStartGlobal()


    def showWidget(self, b):
        """
        Show the widget
        """
        label = b.getLabel()
        self.__boundary = Boundary('coal_inlet', label, self.__case)

        # Initialize velocity
        choice = self.__boundary.getVelocityChoice()
        self.modelVelocity.setItem(str_model=choice)
        self.__updateLabel()

        if choice[-7:] == "formula":
            self.pushButtonVelocityFormula.setEnabled(True)
            self.lineEditVelocity.setEnabled(False)
        else:
            self.pushButtonVelocityFormula.setEnabled(False)
            self.lineEditVelocity.setEnabled(True)
            v = self.__boundary.getVelocity()
            self.lineEditVelocity.setText(QString(str(v)))

        # Initialize oxydant and temperature
        self.spinBoxOxydantNumber.setMaximum(self.__maxOxydantNumber)
        o = self.__boundary.getOxydantNumber()
        self.spinBoxOxydantNumber.setValue(o)
        t = self.__boundary.getOxydantTemperature()
        self.lineEditTemperature.setText(QString(str(t)))

        # Initialize direction
        choice = self.__boundary.getDirectionChoice()
        self.modelDirection.setItem(str_model=choice)
        text = self.modelDirection.dicoM2V[choice]
        if choice == "formula":
            self.pushButtonDirectionFormula.setEnabled(True)
            self.frameDirectionCoordinates.hide()
        elif choice == "coordinates":
            self.pushButtonDirectionFormula.setEnabled(False)
            self.frameDirectionCoordinates.show()
            v = self.__boundary.getDirection('direction_x')
            self.lineEditDirectionX.setText(QString(str(v)))
            v = self.__boundary.getDirection('direction_y')
            self.lineEditDirectionY.setText(QString(str(v)))
            v = self.__boundary.getDirection('direction_z')
            self.lineEditDirectionZ.setText(QString(str(v)))
        elif choice == "normal":
            self.pushButtonDirectionFormula.setEnabled(False)
            self.frameDirectionCoordinates.hide()

        log.debug("showWidget:inlet type: %s " % self.__boundary.getInletType())
        if self.__boundary.getInletType() == "coalFlow":
            self.modelTypeInlet.setItem(str_model="coalFlow")
            self.groupBoxCoal.show()
            self.groupBoxCoalMass.show()
            self.__updateTables()
            self.__boundary.setInletType("coalFlow")
        else:
            self.__boundary.setInletType("oxydantFlow")
            self.modelTypeInlet.setItem(str_model="oxydantFlow")
            self.groupBoxCoal.hide()
            self.groupBoxCoalMass.hide()

        self.show()


    def hideWidget(self):
        """
        Hide all
        """
        self.hide()


    def __updateTables(self):
        """
        Insert rows in the two QTableView.
        """
        # clean the QTableView
        self.__modelCoal.deleteAll()
        self.__modelCoalMass.deleteAll()

        label = self.__boundary.getLabel()
        self.__modelCoalMass.setBoundaryFromLabel(label)
        self.__modelCoal.setBoundaryFromLabel(label)

        # fill the flow and temperature of the coal
        for coal in range(0, self.__coalNumber):
            self.__modelCoal.insertItem(self.tr("Coal ") + " " + str(coal+1),
                                        self.__boundary.getCoalFlow(coal),
                                        self.__boundary.getCoalTemperature(coal))

        # fill the ratio of mass for each class for each coal
        for coal in range(0, self.__coalNumber) :
            lastValue = 0.
            for coalClass in range(0, self.__coalClassesNumber[coal]-1):
                list = self.__boundary.getCoalRatios(coal)
                lastValue += list[coalClass]
                self.__ratio[coal][coalClass] = list[coalClass]

            # last class is computed in order to assure that sum is egal to 100%
            coalClass = self.__coalClassesNumber[coal]-1
            lastValue = 100 - lastValue
            self.__ratio[coal][coalClass] = lastValue

        self.__modelCoalMass.setRatio(self.__ratio)


    @pyqtSignature("const QString&")
    def __slotChoiceVelocity(self, text):
        """
        Private slot.

        Input the velocity boundary type choice (norm, ).

        @type text: C{QString}
        @param text: velocity boundary type choice.
        """
        c = self.modelVelocity.dicoV2M[str(text)]
        log.debug("slotChoiceVelocity: %s " % c)
        self.__boundary.setVelocityChoice(c)

        if c[-7:] == "formula":
            self.pushButtonVelocityFormula.setEnabled(True)
            setGreenColor(self.pushButtonVelocityFormula, True)
            self.lineEditVelocity.setEnabled(False)
            self.lineEditVelocity.setText(QString(""))
        else:
            self.pushButtonVelocityFormula.setEnabled(False)
            setGreenColor(self.pushButtonVelocityFormula, False)
            self.lineEditVelocity.setEnabled(True)
            v = self.__boundary.getVelocity()
            self.lineEditVelocity.setText(QString(str(v)))

        self.__updateLabel()


    def __updateLabel(self):
        """
        Update the unit for the velocity specification.
        """
        c = self.__boundary.getVelocityChoice()
        if c in ('norm', 'norm_formula'):
            self.labelUnitVelocity.setText(QString(str('m/s')))
        elif c in ('flow1', 'flow1_formula'):
            self.labelUnitVelocity.setText(QString(str('kg/s')))
        elif c in ('flow2', 'flow2_formula'):
            self.labelUnitVelocity.setText(QString(str('m<sup>3</sup>/s')))


    @pyqtSignature("const QString&")
    def __slotVelocityValue(self, text):
        """
        Private slot.

        New value associated to the velocity boundary type.

        @type text: C{QString}
        @param text: value
        """
        v, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setVelocity(v)


    @pyqtSignature("")
    def __slotVelocityFormula(self):
        """
        """
        exp = self.__boundary.getVelocity()
        c = self.__boundary.getVelocityChoice()
        req = [('u_norm', 'Norm of the velocity')]
        if c == 'norm_formula':
            exa = "u_norm = 1.0;"
        elif c == 'flow1_formula':
            exa = "q_m = 1.0;"
        elif c == 'flow2_formula':
            exa = "q_v = 1.0;"

        sym = [('x', "X face's gravity center"),
               ('y', "Y face's gravity center"),
               ('z', "Z face's gravity center"),
               ('dt', 'time step'),
               ('t', 'current time'),
               ('iter', 'number of iteration')]

        dialog = QMeiEditorView(self,
                                check_syntax = self.__case['package'].get_check_syntax(),
                                expression = exp,
                                required   = req,
                                symbols    = sym,
                                examples   = exa)
        if dialog.exec_():
            result = dialog.get_result()
            log.debug("slotFormulaVelocity -> %s" % str(result))
            self.__boundary.setVelocity(result)
            setGreenColor(self.pushButtonVelocityFormula, False)


    @pyqtSignature("const QString&")
    def __slotChoiceDirection(self, text):
        """
        Input the direction type choice.
        """
        c = self.modelDirection.dicoV2M[str(text)]
        log.debug("slotChoiceVelocity: %s " % c)
        self.__boundary.setDirectionChoice(c)

        if c == "formula":
            self.pushButtonDirectionFormula.setEnabled(True)
            setGreenColor(self.pushButtonDirectionFormula, True)
            self.frameDirectionCoordinates.hide()
        elif c == "coordinates":
            self.pushButtonDirectionFormula.setEnabled(False)
            setGreenColor(self.pushButtonDirectionFormula, False)
            self.frameDirectionCoordinates.show()
            v = self.__boundary.getDirection('direction_x')
            self.lineEditDirectionX.setText(QString(str(v)))
            v = self.__boundary.getDirection('direction_y')
            self.lineEditDirectionY.setText(QString(str(v)))
            v = self.__boundary.getDirection('direction_z')
            self.lineEditDirectionZ.setText(QString(str(v)))
        elif c == "normal":
            self.pushButtonDirectionFormula.setEnabled(False)
            setGreenColor(self.pushButtonDirectionFormula, False)
            self.frameDirectionCoordinates.hide()


    @pyqtSignature("const QString&")
    def __slotDirX(self, text):
        """
        INPUT value into direction of inlet flow
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setDirection('direction_x', value)


    @pyqtSignature("const QString&")
    def __slotDirY(self, text):
        """
        INPUT value into direction of inlet flow
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setDirection('direction_y', value)


    @pyqtSignature("const QString&")
    def __slotDirZ(self, text):
        """
        INPUT value into direction of inlet flow
        """
        value, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setDirection('direction_z', value)


    @pyqtSignature("")
    def __slotDirectionFormula(self):
        """
        """
        exp = self.__boundary.getDirection('direction_formula')

        req = [('dir_x', 'Direction of the flow along X'),
               ('dir_y', 'Direction of the flow along Y'),
               ('dir_z', 'Direction of the flow along Z')]

        exa = "dir_x = 3.0;\ndir_y = 1.0;\ndir_z = 0.0;\n"

        sym = [('x', "X face's gravity center"),
               ('y', "Y face's gravity center"),
               ('z', "Z face's gravity center"),
               ('dt', 'time step'),
               ('t', 'current time'),
               ('iter', 'number of iteration')]

        dialog = QMeiEditorView(self,
                                check_syntax = self.__case['package'].get_check_syntax(),
                                expression = exp,
                                required   = req,
                                symbols    = sym,
                                examples   = exa)
        if dialog.exec_():
            result = dialog.get_result()
            log.debug("slotFormulaDirection -> %s" % str(result))
            self.__boundary.setDirection('direction_formula', result)
            setGreenColor(self.pushButtonDirectionFormula, False)



    @pyqtSignature("const QString&")
    def __slotInletType(self, text):
        """
        INPUT inlet type : 'oxydant' or 'oxydant + coal'
        """
        value = self.modelTypeInlet.dicoV2M[str(text)]
        log.debug("__slotInletType value = %s " % value)

        self.__boundary.setInletType(value)

        if value == 'oxydantFlow':
            self.groupBoxCoal.hide()
            self.groupBoxCoalMass.hide()
        else:
            self.groupBoxCoal.show()
            self.groupBoxCoalMass.show()
            self.__updateTables()


    @pyqtSignature("const QString&")
    def __slotTemperature(self, text):
        t, ok = text.toDouble()
        if self.sender().validator().state == QValidator.Acceptable:
            self.__boundary.setOxydantTemperature(t)


    @pyqtSignature("int")
    def __slotOxydantNumber(self, i):
        self.__boundary.setOxydantNumber(i)


    def getCoalNumber(self):
        """
        Return the coal number
        """
        return self.__coalNumber


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
