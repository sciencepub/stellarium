/*
 * Stellarium
 * Copyright (C) 2008 Fabien Chereau
 * Copyright (C) 2022 Ruslan Kabatsayev
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "MapWidget.hpp"
#include <cmath>
#include <QPainter>
#include <QMouseEvent>

namespace
{
constexpr int GUI_PIXMAPS_SCALE = 5;
}

MapWidget::MapWidget(QWidget* parent)
	: QWidget(parent)
	, map(":/graphicGui/miscWorldMap.jpg")
	, locationMarker(":/graphicGui/uieMapPointer.png")
	, markerVisible(true)
{
}

void MapWidget::setMarkerVisible(bool visible)
{
	markerVisible=visible;
}

void MapWidget::setMarkerPos(double longitude, double latitude)
{
	markerLat = latitude;
	markerLon = longitude;
	update();
}

void MapWidget::mousePressEvent(QMouseEvent* event)
{
	const double ratio = devicePixelRatioF();
	const auto pos = QPointF(event->pos()) * ratio;

	if(!mapRect.contains(pos))
		return;

	const auto lat = (pos.y() - mapRect.center().y()) / mapRect.height() * -180;
	const auto lon = (pos.x() - mapRect.center().x()) / mapRect.width()  * 360;

	// rescale pos to original map
	QPoint imgPoint((static_cast<float>(pos.x() - mapRect.center().x()) / static_cast<float>(mapRect.width())  +0.5) * map.width(),
			(static_cast<float>(pos.y() - mapRect.center().y()) / static_cast<float>(mapRect.height()) +0.5) * map.height());

	// Sample the map pixel color. Use a small box to avoid 1-pixel surprises.
	QImage sampledPix=map.copy(QRect(imgPoint-QPoint(4,4), QSize(8,8))).toImage().scaled(1,1);
	QColor color=sampledPix.pixelColor(0,0);

	emit positionChanged(lon, lat, color);
}

void MapWidget::setMap(const QPixmap &map)
{
	this->map = map;
	update();
}

void MapWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	const double ratio = devicePixelRatioF();
	painter.scale(1/ratio, 1/ratio); // Work in units of device pixels

	// QPixmap::scaled() gives higher quality of resampling than QPainter::drawPixmap()
	const auto scaledMap = map.scaled(size() * ratio, Qt::KeepAspectRatio, Qt::SmoothTransformation);

	mapRect = QRectF(QPointF(std::round(( width()*ratio-scaledMap.width ()) / 2.),
							 std::round((height()*ratio-scaledMap.height()) / 2.)),
					 scaledMap.size());

	painter.drawPixmap(mapRect.topLeft(), scaledMap);

	if (markerVisible)
	{
		// We want the marker to have odd width, so that the arrow tip is one pixel wide.
		auto locMarkerWidth = int(locationMarker.width() * (ratio / GUI_PIXMAPS_SCALE));
		if(locMarkerWidth % 2 == 0)
			++locMarkerWidth;
		const auto scaledMarker = locationMarker.scaledToWidth(locMarkerWidth, Qt::SmoothTransformation);

		const auto markerCenterPosX = std::lround(mapRect.left() + (markerLon + 180) /  360. * scaledMap.width() );
		const auto markerCenterPosY = std::lround(mapRect.top()  + (markerLat -  90) / -180. * scaledMap.height());

		painter.drawPixmap(markerCenterPosX - scaledMarker.width()/2,
				   markerCenterPosY - scaledMarker.height(),
				   scaledMarker);
	}
}
