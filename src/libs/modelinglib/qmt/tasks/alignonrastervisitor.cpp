// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "alignonrastervisitor.h"

#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/diagram/dannotation.h"
#include "qmt/diagram/dassociation.h"
#include "qmt/diagram/dboundary.h"
#include "qmt/diagram/dclass.h"
#include "qmt/diagram/dcomponent.h"
#include "qmt/diagram/dconnection.h"
#include "qmt/diagram/ddependency.h"
#include "qmt/diagram/ddiagram.h"
#include "qmt/diagram/ditem.h"
#include "qmt/diagram/delement.h"
#include "qmt/diagram/dinheritance.h"
#include "qmt/diagram/dobject.h"
#include "qmt/diagram/dpackage.h"
#include "qmt/diagram/drelation.h"
#include "qmt/diagram/dswimlane.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/tasks/isceneinspector.h"

namespace qmt {

AlignOnRasterVisitor::AlignOnRasterVisitor()
{
}

AlignOnRasterVisitor::~AlignOnRasterVisitor()
{
}

void AlignOnRasterVisitor::setDiagramController(DiagramController *diagramController)
{
    m_diagramController = diagramController;
}

void AlignOnRasterVisitor::setSceneInspector(ISceneInspector *sceneInspector)
{
    m_sceneInspector = sceneInspector;
}

void AlignOnRasterVisitor::setDiagram(MDiagram *diagram)
{
    m_diagram = diagram;
}

void AlignOnRasterVisitor::visitDElement(DElement *element)
{
    Q_UNUSED(element)

    QMT_CHECK(false);
}

void AlignOnRasterVisitor::visitDObject(DObject *object)
{
    IResizable *resizable = m_sceneInspector->resizable(object, m_diagram);
    if (resizable)
        resizable->alignItemSizeToRaster(IResizable::SideRightOrBottom, IResizable::SideRightOrBottom,
                                         2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
    IMoveable *moveable = m_sceneInspector->moveable(object, m_diagram);
    if (moveable)
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
}

void AlignOnRasterVisitor::visitDPackage(DPackage *package)
{
    visitDObject(package);
}

void AlignOnRasterVisitor::visitDClass(DClass *klass)
{
    visitDObject(klass);
}

void AlignOnRasterVisitor::visitDComponent(DComponent *component)
{
    visitDObject(component);
}

void AlignOnRasterVisitor::visitDDiagram(DDiagram *diagram)
{
    visitDObject(diagram);
}

void AlignOnRasterVisitor::visitDItem(DItem *item)
{
    visitDObject(item);
}

void AlignOnRasterVisitor::visitDRelation(DRelation *relation)
{
    Q_UNUSED(relation)
}

void AlignOnRasterVisitor::visitDInheritance(DInheritance *inheritance)
{
    visitDRelation(inheritance);
}

void AlignOnRasterVisitor::visitDDependency(DDependency *dependency)
{
    visitDRelation(dependency);
}

void AlignOnRasterVisitor::visitDAssociation(DAssociation *association)
{
    visitDRelation(association);
}

void AlignOnRasterVisitor::visitDConnection(DConnection *connection)
{
    visitDRelation(connection);
}

void AlignOnRasterVisitor::visitDAnnotation(DAnnotation *annotation)
{
    IMoveable *moveable = m_sceneInspector->moveable(annotation, m_diagram);
    if (moveable)
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
}

void AlignOnRasterVisitor::visitDBoundary(DBoundary *boundary)
{
    IResizable *resizable = m_sceneInspector->resizable(boundary, m_diagram);
    if (resizable)
        resizable->alignItemSizeToRaster(IResizable::SideRightOrBottom, IResizable::SideRightOrBottom,
                                         2 * RASTER_WIDTH, 2 * RASTER_HEIGHT);
    IMoveable *moveable = m_sceneInspector->moveable(boundary, m_diagram);
    if (moveable)
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
}

void AlignOnRasterVisitor::visitDSwimlane(DSwimlane *swimlane)
{
    IMoveable *moveable = m_sceneInspector->moveable(swimlane, m_diagram);
    if (moveable)
        moveable->alignItemPositionToRaster(RASTER_WIDTH, RASTER_HEIGHT);
}

} // namespace qmt
