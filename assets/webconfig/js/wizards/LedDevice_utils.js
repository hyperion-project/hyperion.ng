
const ledDeviceWizardUtils = (() => {

  // Layout positions
  const positionMap = {
    "top": { hmin: 0.15, hmax: 0.85, vmin: 0, vmax: 0.2 },
    "topleft": { hmin: 0, hmax: 0.15, vmin: 0, vmax: 0.15 },
    "topright": { hmin: 0.85, hmax: 1.0, vmin: 0, vmax: 0.15 },
    "bottom": { hmin: 0.15, hmax: 0.85, vmin: 0.8, vmax: 1.0 },
    "bottomleft": { hmin: 0, hmax: 0.15, vmin: 0.85, vmax: 1.0 },
    "bottomright": { hmin: 0.85, hmax: 1.0, vmin: 0.85, vmax: 1.0 },
    "left": { hmin: 0, hmax: 0.15, vmin: 0.15, vmax: 0.85 },
    "lefttop": { hmin: 0, hmax: 0.15, vmin: 0, vmax: 0.5 },
    "leftmiddle": { hmin: 0, hmax: 0.15, vmin: 0.25, vmax: 0.75 },
    "leftbottom": { hmin: 0, hmax: 0.15, vmin: 0.5, vmax: 1.0 },
    "right": { hmin: 0.85, hmax: 1.0, vmin: 0.15, vmax: 0.85 },
    "righttop": { hmin: 0.85, hmax: 1.0, vmin: 0, vmax: 0.5 },
    "rightmiddle": { hmin: 0.85, hmax: 1.0, vmin: 0.25, vmax: 0.75 },
    "rightbottom": { hmin: 0.85, hmax: 1.0, vmin: 0.5, vmax: 1.0 },
    "lightPosBottomLeft14": { hmin: 0, hmax: 0.25, vmin: 0.85, vmax: 1.0 },
    "lightPosBottomLeft12": { hmin: 0.25, hmax: 0.5, vmin: 0.85, vmax: 1.0 },
    "lightPosBottomLeft34": { hmin: 0.5, hmax: 0.75, vmin: 0.85, vmax: 1.0 },
    "lightPosBottomLeft11": { hmin: 0.75, hmax: 1, vmin: 0.85, vmax: 1.0 },
    "lightPosBottomLeft112": { hmin: 0, hmax: 0.5, vmin: 0.85, vmax: 1.0 },
    "lightPosBottomLeft121": { hmin: 0.5, hmax: 1, vmin: 0.85, vmax: 1.0 },
    "lightPosBottomLeftNewMid": { hmin: 0.25, hmax: 0.75, vmin: 0.85, vmax: 1.0 },
    "lightPosTopLeft112": { hmin: 0, hmax: 0.5, vmin: 0, vmax: 0.15 },
    "lightPosTopLeft121": { hmin: 0.5, hmax: 1, vmin: 0, vmax: 0.15 },
    "lightPosTopLeftNewMid": { hmin: 0.25, hmax: 0.75, vmin: 0, vmax: 0.15 },
    "lightPosEntire": { hmin: 0.0, hmax: 1.0, vmin: 0.0, vmax: 1.0 }
  };

  return {

    //return editor Value
    eV: function (vn, defaultVal = "") {
      let editor = null;
      if (vn) {
        editor = conf_editor.getEditor("root.specificOptions." + vn);
      }

      if (editor === null) {
        return defaultVal;
      } else if (defaultVal !== "" && !isNaN(defaultVal) && isNaN(editor.getValue())) {
        return defaultVal;
      } else {
        return editor.getValue();
      }
    },
    assignLightPos: function (pos, name) {
      // Retrieve the corresponding position object from the positionMap
      const i = positionMap[pos] || positionMap["lightPosEntire"];
      i.name = name;
      return i;
    }
  };

})();

export { ledDeviceWizardUtils };
