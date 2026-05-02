.pragma library
.import "ArchitectureEdgeUtils.js" as EdgeUtils

var READABLE_DEBUG_TAG = "readable-component-detail-v10-spaced-dimmed"
var READABLE_DEBUG_ENABLED = true

function readableDebug(prefix, message) {
    if (!READABLE_DEBUG_ENABLED)
        return
    console.log("[READABLE-" + String(prefix) + "]", READABLE_DEBUG_TAG, String(message))
}

function readablePointText(point) {
    if (!point)
        return "<null>"
    return "(" + Math.round(Number(point.x) || 0) + "," + Math.round(Number(point.y) || 0) + ")"
}

function readableRouteText(route) {
    if (!route)
        return "<null-route>"
    var pts = routePoints(route)
    var parts = []
    for (var i = 0; i < pts.length; ++i)
        parts.push(readablePointText(pts[i]))
    return "style=" + String(route.style || "") + " points=" + String(pts.length) + " " + parts.join(" -> ")
}

function segmentLength(startPoint, endPoint) {
    var dx = endPoint.x - startPoint.x
    var dy = endPoint.y - startPoint.y
    return Math.sqrt((dx * dx) + (dy * dy))
}

function pointToward(originPoint, targetPoint, distance) {
    var length = segmentLength(originPoint, targetPoint)
    if (length < 0.001)
        return Qt.point(originPoint.x, originPoint.y)

    var ratio = distance / length
    return Qt.point(originPoint.x + ((targetPoint.x - originPoint.x) * ratio),
                    originPoint.y + ((targetPoint.y - originPoint.y) * ratio))
}

function routePoints(route) {
    if (!route)
        return []
    if (route.points && route.points.length >= 2)
        return route.points
    return [route.p0, route.p1, route.p2, route.p3]
}

function curveLeadControl(route, fromStart) {
    var startPoint = fromStart ? route.p0 : route.p3
    var anchorPoint = fromStart ? route.p1 : route.p2
    var bendPoint = fromStart ? route.p2 : route.p1
    var straightSpan = EdgeUtils.pointDistance(startPoint, anchorPoint)
    var bendSpan = EdgeUtils.pointDistance(anchorPoint, bendPoint)
    var straightRatio = straightSpan > 56 ? 0.94 : (straightSpan > 24 ? 0.86 : 0.7)
    var bendRatio = bendSpan > 88 ? 0.08 : 0.14
    var straightLead = EdgeUtils.interpolatePoint(startPoint, anchorPoint, straightRatio)
    return EdgeUtils.interpolatePoint(straightLead, bendPoint, bendRatio)
}

function routeCornerRadius(route, componentMode, emphasized) {
    if (!route || route.style === "curved")
        return 0

    var baseRadius = componentMode ? 18 : 22
    if (emphasized)
        baseRadius += 4
    return baseRadius
}

function drawRoundedOrthogonalPath(ctx, points, desiredRadius) {
    if (!points || points.length < 2)
        return

    ctx.moveTo(points[0].x, points[0].y)

    if (points.length === 2) {
        ctx.lineTo(points[1].x, points[1].y)
        return
    }

    for (var index = 1; index < points.length - 1; ++index) {
        var previousPoint = points[index - 1]
        var currentPoint = points[index]
        var nextPoint = points[index + 1]
        var previousLength = segmentLength(previousPoint, currentPoint)
        var nextLength = segmentLength(currentPoint, nextPoint)
        var cornerRadius = Math.min(desiredRadius, previousLength / 2, nextLength / 2)

        if (cornerRadius < 1) {
            ctx.lineTo(currentPoint.x, currentPoint.y)
            continue
        }

        var entryPoint = pointToward(currentPoint, previousPoint, cornerRadius)
        var exitPoint = pointToward(currentPoint, nextPoint, cornerRadius)
        ctx.lineTo(entryPoint.x, entryPoint.y)
        ctx.quadraticCurveTo(currentPoint.x, currentPoint.y, exitPoint.x, exitPoint.y)
    }

    var finalPoint = points[points.length - 1]
    ctx.lineTo(finalPoint.x, finalPoint.y)
}

function drawRoutePath(ctx, route, componentMode, emphasized) {
    if (!route) {
        readableDebug("PAINTER", "drawRoutePath skip null-route")
        return
    }
    readableDebug("PAINTER", "drawRoutePath componentMode=" + String(!!componentMode) + " emphasized=" + String(!!emphasized) + " " + readableRouteText(route))

    if (route.style === "curved") {
        var control1 = curveLeadControl(route, true)
        var control2 = curveLeadControl(route, false)
        ctx.moveTo(route.p0.x, route.p0.y)
        ctx.bezierCurveTo(control1.x, control1.y, control2.x, control2.y, route.p3.x, route.p3.y)
        return
    }

    drawRoundedOrthogonalPath(ctx, routePoints(route), routeCornerRadius(route, componentMode, emphasized))
}

function routeArrowTail(route) {
    if (!route) {
        readableDebug("PAINTER", "routeArrowTail null-route")
        return Qt.point(0, 0)
    }
    if (route.style === "curved")
        return curveLeadControl(route, false)

    var points = routePoints(route)
    return points.length >= 2 ? points[points.length - 2] : Qt.point(0, 0)
}

function routeEndPoint(route) {
    var points = routePoints(route)
    var point = points.length >= 1 ? points[points.length - 1] : Qt.point(0, 0)
    readableDebug("PAINTER", "routeEndPoint " + readablePointText(point) + " " + readableRouteText(route))
    return point
}

function routeStartPoint(route) {
    var points = routePoints(route)
    var point = points.length >= 1 ? points[0] : Qt.point(0, 0)
    readableDebug("PAINTER", "routeStartPoint " + readablePointText(point) + " " + readableRouteText(route))
    return point
}

function routeStartTail(route) {
    if (!route) {
        readableDebug("PAINTER", "routeStartTail null-route")
        return Qt.point(0, 0)
    }
    if (route.style === "curved")
        return curveLeadControl(route, true)

    var points = routePoints(route)
    return points.length >= 2 ? points[1] : Qt.point(0, 0)
}

function sortRenderEntries(entries) {
    entries.sort(function(left, right) {
        var leftZ = left.visualState && left.visualState.z !== undefined ? left.visualState.z : 0
        var rightZ = right.visualState && right.visualState.z !== undefined ? right.visualState.z : 0
        if (leftZ !== rightZ)
            return leftZ - rightZ
        return String(left.edge && left.edge.id !== undefined ? left.edge.id : "")
                .localeCompare(String(right.edge && right.edge.id !== undefined ? right.edge.id : ""))
    })
    return entries
}
