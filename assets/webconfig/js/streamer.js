$(document).ready(function () {

  // check if browser supports streaming
  if (window.navigator.mediaDevices && window.navigator.mediaDevices.getDisplayMedia) {
    $("#btn_streamer").toggle();
  }

  // variables
  var streamActive = false;
  var screenshotTimer = "";
  var screenshotIntervalTimeMs = 100;
  var streamImageHeight = 0;
  var streamImageWidth = 0;
  const videoElem = document.getElementById("streamvideo");
  const canvasElem = document.getElementById("streamcanvas");

  // Options for getDisplayMedia()
  var displayMediaOptions = {
    video: {
      cursor: "never",
      width: 170,
      height: 100,
      frameRate: 15
    },
    audio: false
  };

  async function startCapture() {
    streamActive = true;

    try {
      var stream = await navigator.mediaDevices.getDisplayMedia(displayMediaOptions);
      videoElem.srcObject = stream;

      // get the active track of the stream
      const track = stream.getVideoTracks()[0];

      // listen for track ending, fires when user aborts through browser
      track.onended = function (event) {
        stopCapture();
      };

      // wait for video ready
      videoElem.addEventListener('loadedmetadata', (e) => {
        window.setTimeout(() => (
          onCapabilitiesReady(track.getSettings())
        ), 500);
      });
    } catch (err) {
      stopCapture();
      console.error("Error: " + err);
    }
  }

  function onCapabilitiesReady(settings) {
    // extract real width/height
    streamImageWidth = settings.width;
    streamImageHeight = settings.height;

    // start screenshotTimer
    updateScrTimer(false);

    // we are sending
    $("#btn_streamer_icon").addClass("text-danger");
  }

  function stopCapture(evt) {
    streamActive = false;
    $("#btn_streamer_icon").removeClass("text-danger");

    updateScrTimer(true);
    // sometimes it's null on abort
    if (videoElem.srcObject) {
      let tracks = videoElem.srcObject.getTracks();

      tracks.forEach(track => track.stop());
      videoElem.srcObject = null;
    }
    requestPriorityClear(1);
  }

  function takePicture() {
    var context = canvasElem.getContext('2d');
    canvasElem.width = streamImageWidth;
    canvasElem.height = streamImageHeight;
    context.drawImage(videoElem, 0, 0, streamImageWidth, streamImageHeight);

    var data = canvasElem.toDataURL('image/png').split(",")[1];
    requestSetImage(data, -1, "Streaming");
  }

  // start or update screenshot timer
  function updateScrTimer(stop) {
    clearInterval(screenshotTimer)

    if (stop === false) {
      screenshotTimer = setInterval(() => (
        takePicture()
      ), screenshotIntervalTimeMs);
    }
  }

  $("#btn_streamer").off().on("click", function (e) {
    if (!$("#btn_streamer_icon").hasClass("text-danger") && !streamActive) {
      startCapture();
    } else {
      stopCapture();
    }
  });

  $(window.hyperion).on("stopBrowerScreenCapture", function (event) {
    if (streamActive) {
      stopCapture();
    }
  });

});
