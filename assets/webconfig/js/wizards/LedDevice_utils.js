// Layout positions
var lightPosTop = { hmin: 0.15, hmax: 0.85, vmin: 0, vmax: 0.2 };
var lightPosTopLeft = { hmin: 0, hmax: 0.15, vmin: 0, vmax: 0.15 };
var lightPosTopRight = { hmin: 0.85, hmax: 1.0, vmin: 0, vmax: 0.15 };
var lightPosBottom = { hmin: 0.15, hmax: 0.85, vmin: 0.8, vmax: 1.0 };
var lightPosBottomLeft = { hmin: 0, hmax: 0.15, vmin: 0.85, vmax: 1.0 };
var lightPosBottomRight = { hmin: 0.85, hmax: 1.0, vmin: 0.85, vmax: 1.0 };
var lightPosLeft = { hmin: 0, hmax: 0.15, vmin: 0.15, vmax: 0.85 };
var lightPosLeftTop = { hmin: 0, hmax: 0.15, vmin: 0, vmax: 0.5 };
var lightPosLeftMiddle = { hmin: 0, hmax: 0.15, vmin: 0.25, vmax: 0.75 };
var lightPosLeftBottom = { hmin: 0, hmax: 0.15, vmin: 0.5, vmax: 1.0 };
var lightPosRight = { hmin: 0.85, hmax: 1.0, vmin: 0.15, vmax: 0.85 };
var lightPosRightTop = { hmin: 0.85, hmax: 1.0, vmin: 0, vmax: 0.5 };
var lightPosRightMiddle = { hmin: 0.85, hmax: 1.0, vmin: 0.25, vmax: 0.75 };
var lightPosRightBottom = { hmin: 0.85, hmax: 1.0, vmin: 0.5, vmax: 1.0 };
var lightPosEntire = { hmin: 0.0, hmax: 1.0, vmin: 0.0, vmax: 1.0 };

var lightPosBottomLeft14 = { hmin: 0, hmax: 0.25, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft12 = { hmin: 0.25, hmax: 0.5, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft34 = { hmin: 0.5, hmax: 0.75, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft11 = { hmin: 0.75, hmax: 1, vmin: 0.85, vmax: 1.0 };

var lightPosBottomLeft112 = { hmin: 0, hmax: 0.5, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeft121 = { hmin: 0.5, hmax: 1, vmin: 0.85, vmax: 1.0 };
var lightPosBottomLeftNewMid = { hmin: 0.25, hmax: 0.75, vmin: 0.85, vmax: 1.0 };

var lightPosTopLeft112 = { hmin: 0, hmax: 0.5, vmin: 0, vmax: 0.15 };
var lightPosTopLeft121 = { hmin: 0.5, hmax: 1, vmin: 0, vmax: 0.15 };
var lightPosTopLeftNewMid = { hmin: 0.25, hmax: 0.75, vmin: 0, vmax: 0.15 };

function assignLightPos(pos, name) {
  var i = null;

  if (pos === "top")
    i = lightPosTop;
  else if (pos === "topleft")
    i = lightPosTopLeft;
  else if (pos === "topright")
    i = lightPosTopRight;
  else if (pos === "bottom")
    i = lightPosBottom;
  else if (pos === "bottomleft")
    i = lightPosBottomLeft;
  else if (pos === "bottomright")
    i = lightPosBottomRight;
  else if (pos === "left")
    i = lightPosLeft;
  else if (pos === "lefttop")
    i = lightPosLeftTop;
  else if (pos === "leftmiddle")
    i = lightPosLeftMiddle;
  else if (pos === "leftbottom")
    i = lightPosLeftBottom;
  else if (pos === "right")
    i = lightPosRight;
  else if (pos === "righttop")
    i = lightPosRightTop;
  else if (pos === "rightmiddle")
    i = lightPosRightMiddle;
  else if (pos === "rightbottom")
    i = lightPosRightBottom;
  else if (pos === "lightPosBottomLeft14")
    i = lightPosBottomLeft14;
  else if (pos === "lightPosBottomLeft12")
    i = lightPosBottomLeft12;
  else if (pos === "lightPosBottomLeft34")
    i = lightPosBottomLeft34;
  else if (pos === "lightPosBottomLeft11")
    i = lightPosBottomLeft11;
  else if (pos === "lightPosBottomLeft112")
    i = lightPosBottomLeft112;
  else if (pos === "lightPosBottomLeft121")
    i = lightPosBottomLeft121;
  else if (pos === "lightPosBottomLeftNewMid")
    i = lightPosBottomLeftNewMid;
  else if (pos === "lightPosTopLeft112")
    i = lightPosTopLeft112;
  else if (pos === "lightPosTopLeft121")
    i = lightPosTopLeft121;
  else if (pos === "lightPosTopLeftNewMid")
    i = lightPosTopLeftNewMid;
  else
    i = lightPosEntire;

  i.name = name;
  return i;
}
