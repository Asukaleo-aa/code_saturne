# -*- coding: utf-8 -*-

#-------------------------------------------------------------------------------

# This file is part of Code_Saturne, a general-purpose CFD tool.
#
# Copyright (C) 1998-2012 EDF S.A.
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
- StandardItemModelBoundaries
- BoundaryConditionsView
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

from Pages.BoundaryConditionsForm import Ui_BoundaryConditionsForm

from Base.Toolbox import GuiParam
from Base.QtPage import DoubleValidator, ComboModel
from Pages.LocalizationModel import LocalizationModel, Zone
from Pages.Boundary import Boundary
from Pages.MobileMeshModel import MobileMeshModel

#-------------------------------------------------------------------------------
# log config
#-------------------------------------------------------------------------------

logging.basicConfig()
log = logging.getLogger("BoundaryConditionsView")
log.setLevel(GuiParam.DEBUG)

#-------------------------------------------------------------------------------
# StandarItemModel class to display boundaries in a QTreeView
#-------------------------------------------------------------------------------

class StandardItemModelBoundaries(QStandardItemModel):
    def __init__(self):
        QStandardItemModel.__init__(self)
        self.headers = [self.tr("Label"),
                        self.tr("Zone"),
                        self.tr("Nature"),
                        self.tr("Selection criteria")]
        self.setColumnCount(len(self.headers))
        self.dataBoundary = []


    def data(self, index, role):
        if not index.isValid():
            return QVariant()
        if role == Qt.DisplayRole:
            return QVariant(self.dataBoundary[index.row()][index.column()])
        return QVariant()


    def flags(self, index):
        if not index.isValid():
            return Qt.ItemIsEnabled
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable


    def headerData(self, section, orientation, role):
        if orientation == Qt.Horizontal and role == Qt.DisplayRole:
            return QVariant(self.headers[section])
        return QVariant()


    def setData(self, index, value, role):
        self.emit(SIGNAL("dataChanged(const QModelIndex &, const QModelIndex &)"), index, index)
        return True


    def insertItem(self, label, codeNumber, var_nature, local):
        line = [label, codeNumber, var_nature, local]
        self.dataBoundary.append(line)
        row = self.rowCount()
        self.setRowCount(row+1)


    def getItem(self, row):
        return self.dataBoundary[row]

#-------------------------------------------------------------------------------
# Main class
#-------------------------------------------------------------------------------

class BoundaryConditionsView(QWidget, Ui_BoundaryConditionsForm):
    """
    Main boundary conditions view class
    """
    def __init__(self, parent, case):
        """
        Constructor
        """
        QWidget.__init__(self, parent)

        Ui_BoundaryConditionsForm.__init__(self)
        self.setupUi(self)
        self.__case = case

        self.__case.undoStopGlobal()

        # Model and QTreeView for Boundaries

        self.__modelBoundaries = StandardItemModelBoundaries()
        self.treeViewBoundaries.setModel(self.__modelBoundaries)

        # Fill the model with the boundary zone

        if MobileMeshModel(self.__case).getMethod() == "off":
            list = ('wall', 'inlet', 'outlet')
        else:
            list = ('wall', 'inlet', 'outlet', 'symmetry')

        d = LocalizationModel('BoundaryZone', self.__case)
        for zone in d.getZones():
            label = zone.getLabel()
            nature = zone.getNature()
            codeNumber = zone.getCodeNumber()
            local = zone.getLocalization()
            if nature in list:
                self.__modelBoundaries.insertItem(label, codeNumber, nature, local)

        self.connect(self.treeViewBoundaries,
                     SIGNAL("clicked(const QModelIndex &)"),
                     self.__slotSelectBoundary)

        # Set the case for custom widgets
        self.roughWidget.setup(self.__case)
        self.slidingWidget.setup(self.__case)
        self.velocityWidget.setup(self.__case)
        self.turbulenceWidget.setup(self.__case)
        self.compressibleOutletWidget.setup(self.__case)
        self.coalWidget.setup(self.__case)
        self.meteoWidget.setup(self.__case, self.velocityWidget, self.turbulenceWidget)
        self.scalarsWidget.setup(self.__case)
        self.mobileMeshWidget.setup(self.__case)
        self.radiativeWidget.setup(self.__case)
        self.electricalwidget.setup(self.__case)

        self.__hideAllWidgets()

        self.__case.undoStartGlobal()


    @pyqtSignature("const QModelIndex&")
    def __slotSelectBoundary(self, index):
        """
        Select a boundary in the QTreeView.
        """
        label, codeNumber, nature, local = self.__modelBoundaries.getItem(index.row())
        log.debug("slotSelectBoundary label %s (%s)" % (label, nature))

        self.__hideAllWidgets()
        boundary = Boundary(nature, label, self.__case)

        if nature == 'wall':
            self.__selectWallBoundary(boundary)
        elif nature == 'inlet':
            self.__selectInletBoundary(boundary)
        elif nature == 'outlet':
            self.__selectOutletBoundary(boundary)
        elif nature == 'symmetry':
            self.__selectSymmetryBoundary(boundary)


    def __selectInletBoundary(self, boundary):
        """
        Shows widgets for inlet.
        """
        if self.coalWidget.getCoalNumber() == 0:
            self.velocityWidget.showWidget(boundary)
            self.coalWidget.hideWidget()
        else:
            self.velocityWidget.hideWidget()
            self.coalWidget.showWidget(boundary)

        self.turbulenceWidget.showWidget(boundary)
        self.meteoWidget.showWidget(boundary)
        self.scalarsWidget.showWidget(boundary)
        self.mobileMeshWidget.showWidget(boundary)
        self.electricalwidget.showWidget(boundary)


    def __selectWallBoundary(self, boundary):
        """
        Shows widgets for wall.
        """
        self.slidingWidget.showWidget(boundary)
        self.roughWidget.showWidget(boundary)
        self.scalarsWidget.showWidget(boundary)
        self.mobileMeshWidget.showWidget(boundary)
        self.radiativeWidget.showWidget(boundary)
        self.electricalwidget.showWidget(boundary)


    def __selectOutletBoundary(self, boundary):
        """
        Shows widgets for wall.
        """
        self.scalarsWidget.showWidget(boundary)
        self.mobileMeshWidget.showWidget(boundary)
        self.meteoWidget.showWidget(boundary)
        if self.compressibleOutletWidget.getCompressibleModel() != "off":
            self.compressibleOutletWidget.showWidget(boundary)
        else:
            self.compressibleOutletWidget.hideWidget()
        self.electricalwidget.showWidget(boundary)
        #self.pressureWidget.showWidget(boundary)


    def __selectSymmetryBoundary(self, boundary):
        """
        Shows widgets for wall.
        """
        self.mobileMeshWidget.showWidget(boundary)


    def __hideAllWidgets(self):
        """
        Hides all promoted QWidgets, for all nature.
        """
        self.slidingWidget.hideWidget()
        self.roughWidget.hideWidget()
        self.velocityWidget.hideWidget()
        self.turbulenceWidget.hideWidget()
        self.coalWidget.hideWidget()
        self.compressibleOutletWidget.hideWidget()
        self.meteoWidget.hideWidget()
        self.scalarsWidget.hideWidget()
        self.mobileMeshWidget.hideWidget()
        self.radiativeWidget.hideWidget()
        self.electricalwidget.hideWidget()
        #self.pressureWidget.hideWidget()


    def tr(self, text):
        """
        Translation
        """
        return text

#-------------------------------------------------------------------------------
# End
#-------------------------------------------------------------------------------
