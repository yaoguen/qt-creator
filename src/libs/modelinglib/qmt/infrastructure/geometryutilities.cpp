// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "geometryutilities.h"

#include "qmt/infrastructure/qmtassert.h"

#include <QPolygonF>
#include <QLineF>
#include <QPointF>
#include <QPair>
#include <QList>
#include <QVector2D>
#include <qmath.h>
#include <qdebug.h>
#include <limits>

template <typename T>
inline int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

namespace qmt {

QLineF GeometryUtilities::stretch(const QLineF &line, double p1Extension, double p2Extension)
{
    QLineF direction = line.unitVector();
    QPointF stretchedP1 = line.p1() - (direction.p2() - direction.p1()) * p1Extension;
    QPointF stretchedP2 = line.p2() + (direction.p2() - direction.p1()) * p2Extension;
    return QLineF(stretchedP1, stretchedP2);
}

bool GeometryUtilities::intersect(const QPolygonF &polygon, const QLineF &line,
                                  QPointF *intersectionPoint, QLineF *intersectionLine,
                                  int nearestPoint)
{
    bool found = false;
    qreal mindist = 0;
    QPointF ipoint;
    QLineF iline;
    for (int i = 0; i <= polygon.size() - 2; ++i) {
        QLineF polygonLine(polygon.at(i), polygon.at(i+1));
        QPointF point;
        QLineF::IntersectType intersectionType = polygonLine.intersects(line, &point);
        if (intersectionType == QLineF::BoundedIntersection) {
            qreal dist = QLineF(point, nearestPoint <= 0 ? line.p1() : line.p2()).length();
            if (!found || dist < mindist) {
                mindist = dist;
                ipoint = point;
                iline = polygonLine;
                found = true;
            }
        }
    }
    if (found) {
        if (intersectionPoint)
            *intersectionPoint = ipoint;
        if (intersectionLine)
            *intersectionLine = iline;
    }
    return found;
}

bool GeometryUtilities::intersect(const QList<QPolygonF> &polygons, const QLineF &line,
                                  int *intersectionPolygon, QPointF *intersectionPoint,
                                  QLineF *intersectionLine, int nearestPoint)
{
    bool found = false;
    qreal mindist = 0;
    int ipolygon = -1;
    QPointF ipoint;
    QLineF iline;
    for (int p = 0; p < polygons.size(); ++p) {
        const QPolygonF polygon = polygons.at(p);
        for (int i = 0; i <= polygon.size() - 2; ++i) {
            const QLineF polygonLine(polygon.at(i), polygon.at(i + 1));
            QPointF point;
            QLineF::IntersectType intersectionType = polygonLine.intersects(line, &point);
            if (intersectionType == QLineF::BoundedIntersection) {
                qreal dist = QLineF(point, nearestPoint <= 0 ? line.p1() : line.p2()).length();
                if (!found || dist < mindist) {
                    mindist = dist;
                    ipolygon = p;
                    ipoint = point;
                    iline = polygonLine;
                    found = true;
                }
            }
        }
    }
    if (found) {
        if (intersectionPolygon)
            *intersectionPolygon = ipolygon;
        if (intersectionPoint)
            *intersectionPoint = ipoint;
        if (intersectionLine)
            *intersectionLine = iline;
    }
    return found;
}


namespace {

class Candidate
{
public:
    Candidate() = default;
    Candidate(const QVector2D &f, const QPointF &s, GeometryUtilities::Side t) : first(f), second(s), third(t) { }

    QVector2D first;
    QPointF second;
    GeometryUtilities::Side third = GeometryUtilities::SideUnspecified;
};

}

bool GeometryUtilities::placeRectAtLine(const QRectF &rect, const QLineF &line, double lineOffset, double distance,
                                        const QLineF &intersectionLine, QPointF *placement, Side *horizontalAlignedSide)
{
    QMT_ASSERT(placement, return false);

    QList<Candidate> candidates;
    candidates << Candidate(QVector2D(rect.topRight() - rect.topLeft()), QPointF(0.0, 0.0), SideTop)
               << Candidate(QVector2D(rect.topLeft() - rect.topRight()), rect.topRight() - rect.topLeft(), SideTop)
               << Candidate(QVector2D(rect.bottomLeft() - rect.topLeft()), QPointF(0.0, 0.0), SideLeft)
               << Candidate(QVector2D(rect.topLeft() - rect.bottomLeft()), rect.bottomLeft() - rect.topLeft(), SideLeft)
               << Candidate(QVector2D(rect.bottomRight() - rect.bottomLeft()), rect.bottomLeft() - rect.topLeft(), SideBottom)
               << Candidate(QVector2D(rect.bottomLeft() - rect.bottomRight()), rect.bottomRight() - rect.topLeft(), SideBottom)
               << Candidate(QVector2D(rect.bottomRight() - rect.topRight()), rect.topRight() - rect.topLeft(), SideRight)
               << Candidate(QVector2D(rect.topRight() - rect.bottomRight()), rect.bottomRight() - rect.topLeft(), SideRight);

    QVector<QVector2D> rectEdgeVectors;
    rectEdgeVectors << QVector2D(rect.topLeft() - rect.topLeft())
                    << QVector2D(rect.topRight() - rect.topLeft())
                    << QVector2D(rect.bottomLeft() - rect.topLeft())
                    << QVector2D(rect.bottomRight() -rect.topLeft());

    QVector2D directionVector(line.p2() - line.p1());
    directionVector.normalize();

    QVector2D sideVector(directionVector.y(), -directionVector.x());

    QVector2D intersectionVector(intersectionLine.p2() - intersectionLine.p1());
    intersectionVector.normalize();

    QVector2D outsideVector = QVector2D(intersectionVector.y(), -intersectionVector.x());
    double p = QVector2D::dotProduct(directionVector, outsideVector);
    if (p < 0.0)
        outsideVector = outsideVector * -1.0;

    double smallestA = -1.0;
    QPointF rectTranslation;
    Side side = SideUnspecified;
    int bestSign = 0;

    foreach (const Candidate &candidate, candidates) {
        // solve equation a * directionVector + candidate.first = b * intersectionVector to find smallest a
        double r = directionVector.x() * intersectionVector.y() - directionVector.y() * intersectionVector.x();
        if (r <= -1e-5 || r >= 1e-5) {
            double a = (candidate.first.y() * intersectionVector.x()
                        - candidate.first.x() * intersectionVector.y()) / r;
            if (a >= 0.0 && (smallestA < 0.0 || a < smallestA)) {
                // verify that all rectangle edges lay outside of shape (by checking for positiv projection to intersection)
                bool ok = true;
                int sign = 0;
                QVector2D rectOriginVector = directionVector * a - QVector2D(candidate.second);
                foreach (const QVector2D &rectEdgeVector, rectEdgeVectors) {
                    QVector2D edgeVector = rectOriginVector + rectEdgeVector;
                    double aa = QVector2D::dotProduct(outsideVector, edgeVector);
                    if (aa < 0.0) {
                        ok = false;
                        break;
                    }
                    int s = sgn(QVector2D::dotProduct(sideVector, edgeVector));
                    if (s) {
                        if (sign) {
                            if (s != sign) {
                                ok = false;
                                break;
                            }
                        } else {
                            sign = s;
                        }
                    }
                }
                if (ok) {
                    smallestA = a;
                    rectTranslation = candidate.second;
                    side = candidate.third;
                    bestSign = sign;
                }
            }
        }
    }
    if (horizontalAlignedSide) {
        // convert side into a horizontal side depending on placement relative to direction vector
        switch (side) {
        case SideTop:
            side = bestSign == -1 ? SideRight : SideLeft;
            break;
        case SideBottom:
            side = bestSign == -1 ? SideLeft : SideRight;
            break;
        default:
            break;
        }
        *horizontalAlignedSide = side;
    }
    if (smallestA < 0.0)
        return false;
    *placement = line.p1() + (directionVector * (smallestA + lineOffset)).toPointF()
            + (sideVector * (bestSign * distance)).toPointF() - rectTranslation;
    return true;
}

double GeometryUtilities::calcAngle(const QLineF &line)
{
    QVector2D directionVector(line.p2() - line.p1());
    directionVector.normalize();
    double angle = qAcos(directionVector.x()) * 180.0 / M_PI;
    if (directionVector.y() > 0.0)
        angle = -angle;
    return angle;
}

namespace {

// scalar product
qreal operator&(const QVector2D &lhs, const QVector2D &rhs) {
    return lhs.x() * rhs.x() + lhs.y() * rhs.y();
}

}

double GeometryUtilities::calcDistancePointToLine(const QPointF &point, const QLineF &line)
{
    QVector2D p(point);
    QVector2D a(line.p1());
    QVector2D directionVector(line.p2() - line.p1());
    qreal r = -((a - p) & directionVector) / directionVector.lengthSquared();
    if (r < 0.0 || r > 1.0)
        return std::numeric_limits<float>::quiet_NaN();
    qreal d = (a + r * directionVector - p).length();
    return d;
}

QPointF GeometryUtilities::calcProjection(const QLineF &line, const QPointF &point)
{
    QVector2D p(point);
    QVector2D a(line.p1());
    QVector2D directionVector(line.p2() - line.p1());
    qreal r = -((a - p) & directionVector) / directionVector.lengthSquared();
    return (a + r * directionVector).toPointF();
}

QPointF GeometryUtilities::calcPrimaryAxisDirection(const QLineF &line)
{
    qreal xAbs = qAbs(line.dx());
    qreal yAbs = qAbs(line.dy());
    if (yAbs > xAbs) {
        if (line.dy() >= 0.0)
            return QPointF(0.0, 1.0);
        else
            return QPointF(0.0, -1.0);
    } else {
        if (line.dx() >= 0.0)
            return QPointF(1.0, 0.0);
        else
            return QPointF(-1.0, 0.0);
    }
}

QPointF GeometryUtilities::calcSecondaryAxisDirection(const QLineF &line)
{
    qreal xAbs = qAbs(line.dx());
    qreal yAbs = qAbs(line.dy());
    if (yAbs > xAbs) {
        if (line.dx() >= 0.0)
            return QPointF(1.0, 0.0);
        else
            return QPointF(-1.0, 0.0);
    } else {
        if (line.dy() >= 0.0)
            return QPointF(0.0, 1.0);
        else
            return QPointF(0.0, -1.0);
    }
}

void GeometryUtilities::adjustPosAndRect(QPointF *pos, QRectF *rect, const QPointF &topLeftDelta,
                                         const QPointF &bottomRightDelta, const QPointF &relativeAlignment)
{
    *pos += QPointF(topLeftDelta.x() * (1.0 - relativeAlignment.x()) + bottomRightDelta.x() * relativeAlignment.x(),
                    topLeftDelta.y() * (1.0 - relativeAlignment.y()) + bottomRightDelta.y() * relativeAlignment.y());
    rect->adjust(topLeftDelta.x() * relativeAlignment.x() - bottomRightDelta.x() * relativeAlignment.x(),
                 topLeftDelta.y() * relativeAlignment.y() - bottomRightDelta.y() * relativeAlignment.y(),
                 bottomRightDelta.x() * (1.0 - relativeAlignment.x()) - topLeftDelta.x() * (1.0 - relativeAlignment.x()),
                 bottomRightDelta.y() * (1.0 - relativeAlignment.y()) - topLeftDelta.y() * (1.0 - relativeAlignment.y()));
}

QSizeF GeometryUtilities::ensureMinimumRasterSize(const QSizeF &size, double rasterWidth, double rasterHeight)
{
    double width = int(size.width() / rasterWidth + 0.99999) * rasterWidth;
    double height = int(size.height() / rasterHeight + 0.99999) * rasterHeight;
    return QSizeF(width, height);
}

} // namespace qmt

