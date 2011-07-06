# -*- coding: utf-8 -*-
#
#-------------------------------------------------------------------------------
#
#     This file is part of the Code_Saturne User Interface, element of the
#     Code_Saturne CFD tool.
#
#     Copyright (C) 1998-2007 EDF S.A., France
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
- StandardItemModelVolumicNames
- StandardItemModelBoundariesNames
- LagrangianStatisticsView
"""

#-------------------------------------------------------------------------------
# Standard modules
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


from Pages.LagrangianStatisticsForm import Ui_LagrangianStatisticsForm
from Base.Toolbox import GuiParam
from Base.QtPage import ComboModel, IntValidator, DoubleValidator
from Pages.LagrangianStatisticsModel import LagrangianStatisticsModel
from Pages.LagrangianModel import LagrangianModel


#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------


logging.basicConfig()
log = logging.getLogger("LagrangianStatisticsView")
log.setLevel(GuiParam.DEBUG)


#-------------------------------------------------------------------------------
# StandarItemModel for volumic variables names
#-------------------------------------------------------------------------------


class StandardItemModelVolumicNames(QStandardItemModel):
    def __init__(self, model):
        """
        """
        QStandardItemModel.__init__(self)

        self.headers = [self.tr("Name"),
                        self.tr("Mean value name"),
                        self.tr("Variance name"),
                        self.tr("Recording")]
        self.setColumnCount(len(self.headers))
        self.model = model
        self.initData()


    def initData(self):

        self.dataVolumicNames = []
        vnames = self.model.getVariablesNamesVolume()

        for vname in vnames:
            if vname == "statistical_weight":
                label = self.model.getPropertyLabelFromNameVolume(vname)
                labelv = ""
                monitoring = self.model.getMonitoringStatusFromName(vname)
                line = [vname, label, labelv, monitoring]
            else:
                label = self.model.getPropertyLabelFromNameVolume("mean_" + vname)
                labelv = self.model.getPropertyLabelFromNameVolume("variance_" + vname)
                monitoring = self.model.getMonitoringStatusFromName(label)
                line = [vname, label, labelv, monitoring]

            row = self.rowCount()
            self.setRowCount(row+1)
            self.dataVolumicNames.append(line)


    def data(self, index, role):

        self.kwords = [ "", "NOMLAG", "NOMLAV", "IHSLAG"]
        if not index.isValid():
            return QVariant()

        # ToolTips
        if role == Qt.ToolTipRole:
            if index.column() == 0:
                return QVariant()
            else:
                return QVariant(self.tr("Code_Saturne key word: " + self.kwords[index.column()]))

        # Display
        if role == Qt.DisplayRole:
            if index.column() in [0,1,2]:
                return QVariant(self.dataVolumicNames[index.row()][index.column()])

        # CheckState
        elif role == Qt.CheckStateRole:
            if index.column() == 3:
                if self.dataVolumicNames[index.row()][index.column()] == 'on':
                    return QVariant(Qt.Checked)
                else:
                    return QVariant(Qt.Unchecked)

        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.column() == 0:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        elif index.column() in [1,2] :
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsUserCheckable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        #
        if index.column() == 1:
            label = str(value.toString())
            self.dataVolumicNames[index.row()][index.column()] = label

            vname = self.dataVolumicNames[index.row()][0]
            if index.row() != 0: vname = "mean_" + vname
            self.model.setPropertyLabelFromNameVolume(vname, label)

        elif index.column() == 2:
            labelv = str(value.toString())
            self.dataVolumicNames[index.row()][index.column()] = labelv
            name = self.dataVolumicNames[index.row()][0]
            vname = "variance_" + name
            self.model.setPropertyLabelFromNameVolume(vname, labelv)

        elif index.column() == 3:
            v, ok = value.toInt()
            if v == Qt.Unchecked:
                status = "off"
                self.dataVolumicNames[index.row()][index.column()] = "off"
            else:
                status = "on"
                self.dataVolumicNames[index.row()][index.column()] = "on"

            vname = self.dataVolumicNames[index.row()][0]
            if index.row() != 0: vname = "mean_" + vname
            self.model.setMonitoringStatusFromName(vname, status)

        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


#-------------------------------------------------------------------------------
# StandarItemModel for boundaries variables names
#-------------------------------------------------------------------------------


class StandardItemModelBoundariesNames(QStandardItemModel):
    def __init__(self, model):
        """
        """
        QStandardItemModel.__init__(self)

        self.headers = [self.tr("Name"),
                        self.tr("Value name"),
                        self.tr("Listing"),
                        self.tr("Post-processing")]
        self.setColumnCount(len(self.headers))
        self.model = model
        self.initData()


    def initData(self):

        self.dataBoundariesNames = []
        vnames = self.model.getVariablesNamesBoundary()
        for vname in vnames:
            label   = self.model.getPropertyLabelFromNameBoundary(vname)
            listing = self.model.getListingPrintingStatusFromName(vname)
            postproc = self.model.getPostprocessingStatusFromName(vname)
            line = [vname, label, listing, postproc]

            row = self.rowCount()
            self.setRowCount(row+1)
            self.dataBoundariesNames.append(line)


    def data(self, index, role):

        self.kwords = [ "INBRBD", "IFLMBD", "IANGBD", "IVITBD", "IENCBD"]
        if not index.isValid():
            return QVariant()

        # ToolTips
        if role == Qt.ToolTipRole:
            if index.column() == 1:
                return QVariant(self.tr("Code_Saturne key word: NOMBRD"))
            elif index.column() in [2,3]:
                return QVariant(self.tr("Code_Saturne key word: " + self.kwords[index.row()]))

        # Display
        if role == Qt.DisplayRole:
            if index.column() in [0, 1]:
                return QVariant(self.dataBoundariesNames[index.row()][index.column()])

        # CheckState
        elif role == Qt.CheckStateRole:
            if index.column() in [2, 3]:
                if self.dataBoundariesNames[index.row()][index.column()] == 'on':
                    return QVariant(Qt.Checked)
                else:
                    return QVariant(Qt.Unchecked)

        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        elif index.column() == 0:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable
        elif index.column() == 1 :
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsUserCheckable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):

        if index.column() == 1:
            label = str(value.toString())
            self.dataBoundariesNames[index.row()][index.column()] = label

            vname = self.dataBoundariesNames[index.row()][0]
            self.model.setPropertyLabelFromNameBoundary(vname, label)

        elif index.column() == 2:
            v, ok = value.toInt()
            if v == Qt.Unchecked:
                status = "off"
                self.dataBoundariesNames[index.row()][index.column()] = "off"
            else:
                status = "on"
                self.dataBoundariesNames[index.row()][index.column()] = "on"

            vname = self.dataBoundariesNames[index.row()][0]
            self.model.setListingPrintingStatusFromName(vname, status)

        elif index.column() == 3:
            v, ok = value.toInt()
            if v == Qt.Unchecked:
                status = "off"
                self.dataBoundariesNames[index.row()][index.column()] = "off"
            else:
                status = "on"
                self.dataBoundariesNames[index.row()][index.column()] = "on"

            vname = self.dataBoundariesNames[index.row()][0]
            self.model.setPostprocessingStatusFromName(vname, status)


        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------


class LagrangianStatisticsView(QWidget, Ui_LagrangianStatisticsForm):
    """
    """

    def __init__(self, parent, case):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_LagrangianStatisticsForm.__init__(self)
        self.setupUi(self)

        self.case = case
        self.model = LagrangianStatisticsModel(self.case)

        self.connect(self.checkBoxISUIST, SIGNAL("clicked()"), self.slotISUIST)
        self.connect(self.lineEditNBCLST, SIGNAL("textChanged(const QString &)"), self.slotNBCLST)

        self.connect(self.groupBoxISTALA, SIGNAL("clicked()"), self.slotISTALA)
        self.connect(self.lineEditIDSTNT, SIGNAL("textChanged(const QString &)"), self.slotIDSTNT)
        self.connect(self.lineEditSEUIL,  SIGNAL("textChanged(const QString &)"), self.slotSEUIL)

        self.connect(self.groupBoxIENSI3, SIGNAL("clicked()"), self.slotIENSI3)
        self.connect(self.lineEditNSTBOR, SIGNAL("textChanged(const QString &)"), self.slotNSTBOR)
        self.connect(self.lineEditSEUILF, SIGNAL("textChanged(const QString &)"), self.slotSEUILF)

        validatorNBCLST = IntValidator(self.lineEditNBCLST, min=0) # max=100
        #validatorNBCLST.setExclusiveMin(True)

        validatorIDSTNT = IntValidator(self.lineEditIDSTNT, min=0)
        validatorIDSTNT.setExclusiveMin(True)
        validatorSEUIL = DoubleValidator(self.lineEditSEUIL, min=0.)
        validatorNSTBOR = IntValidator(self.lineEditNSTBOR, min=0)
        validatorNSTBOR.setExclusiveMin(True)
        validatorSEUILF = DoubleValidator(self.lineEditSEUILF, min=0.)

        self.lineEditNBCLST.setValidator(validatorNBCLST)
        self.lineEditIDSTNT.setValidator(validatorIDSTNT)
        self.lineEditSEUIL.setValidator(validatorSEUIL)
        self.lineEditNSTBOR.setValidator(validatorNSTBOR)
        self.lineEditSEUILF.setValidator(validatorSEUILF)

        # initialize Widgets
        # FIXME
        # test if restart lagrangian is on
##         mdl.lagr = LagrangianModel()
##         is_restart = mdl_lagr.getRestart()
##         if is_restart == "off":
##             self.lineEditISUIST.setEnabled(False)
##             self.checkBoxISUIST.setEnabled(False)
        status = self.model.getRestartStatisticsStatus()
        if status == "on":
            self.checkBoxISUIST.setChecked(True)
        else:
            self.checkBoxISUIST.setChecked(False)

        nclust = self.model.getGroupOfParticlesValue()
        self.lineEditNBCLST.setText(QString(str(nclust)))

        # volume
        status = self.model.getVolumeStatisticsStatus()
        if status == "on":
            self.groupBoxISTALA.setChecked(True)
        else:
            self.groupBoxISTALA.setChecked(False)
        self.slotISTALA()

        # boundary
        status = self.model.getBoundaryStatisticsStatus()
        if status == "on":
            self.groupBoxIENSI3.setChecked(True)
        else:
            self.groupBoxIENSI3.setChecked(False)
        self.slotIENSI3()


    def _initVolumicNames(self):
        """
        Initialize names for volumic statistics.
        """
        self.modelVolumicNames = StandardItemModelVolumicNames(self.model)

        self.tableViewVolumicNames.setModel(self.modelVolumicNames)
        self.tableViewVolumicNames.setAlternatingRowColors(True)
        self.tableViewVolumicNames.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.tableViewVolumicNames.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.tableViewVolumicNames.setEditTriggers(QAbstractItemView.DoubleClicked)
        self.tableViewVolumicNames.horizontalHeader().setResizeMode(QHeaderView.Stretch)


    def _initBoundariesNames(self):
        """
        Initialize names for volumic statistics.
        """
        self.modelBoundariesNames = StandardItemModelBoundariesNames(self.model)

        self.tableViewBoundariesNames.setModel(self.modelBoundariesNames)
        self.tableViewBoundariesNames.setAlternatingRowColors(True)
        self.tableViewBoundariesNames.setSelectionBehavior(QAbstractItemView.SelectItems)
        self.tableViewBoundariesNames.setSelectionMode(QAbstractItemView.ExtendedSelection)
        self.tableViewBoundariesNames.setEditTriggers(QAbstractItemView.DoubleClicked)
        self.tableViewBoundariesNames.horizontalHeader().setResizeMode(QHeaderView.Stretch)


    @pyqtSignature("")
    def slotISUIST(self):
        """
        Input ISUIST.
        """
        if self.checkBoxISUIST.isChecked():
            status = "on"
        else:
            status = "off"
        self.model.setRestartStatisticsStatus(status)


    @pyqtSignature("const QString")
    def slotNBCLST(self, text):
        """
        Input NBCLST.
        """
        if self.sender().validator().state == QValidator.Acceptable:
            value, ok = text.toInt()
            self.model.setGroupOfParticlesValue(value)


    @pyqtSignature("")
    def slotISTALA(self):
        """
        Input ISTALA.
        """
        if self.groupBoxISTALA.isChecked():

            self.model.setVolumeStatisticsStatus("on")
            self._initVolumicNames()

            it = self.model.getIterationStartVolume()
            self.lineEditIDSTNT.setText(QString(str(it)))

            seuil = self.model.getThresholdValueVolume()
            self.lineEditSEUIL.setText(QString(str(seuil)))

        else:

            self.model.setVolumeStatisticsStatus("off")
            if hasattr(self, "modelVolumicNames"):
                del self.modelVolumicNames


    @pyqtSignature("const QString&")
    def slotIDSTNT(self, text):
        """
        Input IDSTNT.
        """
        if self.sender().validator().state == QValidator.Acceptable:
            value, ok = text.toInt()
            self.model.setIterationStartVolume(value)


    @pyqtSignature("const QString&")
    def slotSEUIL(self, text):
        """
        Input SEUIL.
        """
        if self.sender().validator().state == QValidator.Acceptable:
            value, ok = text.toDouble()
            self.model.setThresholdValueVolume(value)


    @pyqtSignature("")
    def slotIENSI3(self):
        """
        Input IENSI3.
        """
        if self.groupBoxIENSI3.isChecked():

            self.model.setBoundaryStatisticsStatus("on")
            self._initBoundariesNames()

            it = self.model.getIterationStartBoundary()
            self.lineEditNSTBOR.setText(QString(str(it)))

            seuil = self.model.getThresholdValueBoundary()
            self.lineEditSEUILF.setText(QString(str(seuil)))

        else:

            self.model.setBoundaryStatisticsStatus("off")
            if hasattr(self, "modelBoundariesNames"):
                del self.modelBoundariesNames


    @pyqtSignature("const QString&")
    def slotNSTBOR(self, text):
        """
        Input NSTBOR.
        """
        if self.sender().validator().state == QValidator.Acceptable:
            value, ok = text.toInt()
            self.model.setIterationStartBoundary(value)


    @pyqtSignature("const QString&")
    def slotSEUILF(self, text):
        """
        Input SEUILF.
        """
        if self.sender().validator().state == QValidator.Acceptable:
            value, ok = text.toDouble()
            self.model.setThresholdValueBoundary(value)


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
