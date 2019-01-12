function snippets(options){
// 	"use strict";

	// globals
	var player = this;
	var video = player.el().querySelector('video');
	// hidden canvas for cropping and resizing
	var canvas = document.createElement('canvas');
	// globals for cropping and resizing
	var sx, sy, sw, sh;
	var dx, dy, dw, dh;
	var scale=1;
	var recorder;
	var recording=false; // recording flag, currently recording?

	// switch to recording
	// added to player object to be callable from outside, e.g. shortcut
	player.snippet = function(){
		player.pause();
		// loose keyboard focus
		player.el().blur();
		// switch control bar to recording controls
		player.controlBar.hide();
		recordCtrl.show();
		
		var context = canvas.getContext('2d');
		// crop & scale test
		// initially: everything
		sx=0; sy=0; sw=video.videoWidth; sh=video.videoHeight;
		dx=0; dy=0; dw=sw; dh=sh;
		// resize canvas
		canvas.width = dw;
		canvas.height = dh;
		//TODO: cropping

		// draw video to canvas
		// use requestAnimationFrame to render the video as often as possible
		var draw = function(){
			// schedule next call to this function
			/*if(recording)*/ requestAnimationFrame(draw);
			// draw video data into the canvas
			context.drawImage(video, sx, sy, sw, sh, dx, dy, dw, dh);
		};
		// Start the animation loop
		requestAnimationFrame(draw);

		// edit video in canvas, get stream of edited video
		var canvasStream = canvas.captureStream(15);

		// create one stream out of edited video and audio later if available
		var stream = new MediaStream();
		stream.addTrack(canvasStream.getVideoTracks()[0]);

		if(video.captureStream || video.mozCaptureStream){
			// audio has to play and be recorded, done in audio API
			var audioContext = new AudioContext();
			// use standard video.captureStream() when implemented
			// check for support now, and use what's possible
			video.captureStream = video.captureStream || video.mozCaptureStream;
			var sourceNode = audioContext.createMediaStreamSource(
				//video.mozCaptureStream(15) // firefox specific
				video.captureStream(15)
			);
			sourceNode.connect(audioContext.destination); //let it play unchanged
			var streamDestination = audioContext.createMediaStreamDestination();
			sourceNode.connect(streamDestination); //copy audio for capture
			var directStream = streamDestination.stream;

			stream.addTrack(directStream.getAudioTracks()[0]);
		}

		// recorder with constant(?) bitrate
		recorder = new MediaRecorder(stream, {'videoBitsPerSecond': 1000000});
		recorder.addEventListener('dataavailable', function(e){
			var videoData = [ e.data ];
			var blob = new Blob(videoData, { 'type': 'video/webm'});
			var videoURL = URL.createObjectURL(blob);
			// display download link
			var fname = "test.webm";
			/*var a = document.createElement('a');
			a.download = fname;
			a.href = videoURL;
			a.textContent = 'Click here to download ' + fname + "!";
			if(dl){
				document.getElementById('download').replaceChild(a, dl);
			}else{
				document.getElementById('download').appendChild(a);
			}
			dl = a;*/
			// open a new window with a download link
			var dlWindow = window.open("");
			dlWindow.document.write('<html><head><title>snippet</title></head><body><p>Save video</p><a download="'+fname+'" href="'+videoURL+'">Download</a></body></html>');
		});
	};
	// icon on normal player control bar
	var snippet_btn = player.controlBar.addChild('button');
	snippet_btn.addClass("vjs-snippet-button");
	snippet_btn.el().title = "Switch to WebM recording";
	snippet_btn.on('click', player.snippet);

	//record control bar
	var recordCtrl = player.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl(null, {
				className: 'vjs-control-bar vjs-snippet-ctrl',
			}),
		})
	);
	recordCtrl.hide();

	//TODO: resize

	// crop selection - only record selected area, crop rest
	var crop = recordCtrl.addChild('button');
	crop.addClass("vjs-snippet-crop");
	crop.el().title = "crop video";
	//TODO
	
	// start/stop recording
	var clap = recordCtrl.addChild('button');
	clap.addClass("vjs-snippet-clap");
	clap.el().title = "start/stop recording";
	//TODO
	clap.on('click', function(){
		recording = !recording;
		if(recording){
			//document.getElementById('crop').disabled = true;
			//this.innerHTML = "stop recording";
			recorder.start();
			video.play();
		}else{
			//document.getElementById('crop').disabled = false;
			//this.innerHTML = "start recording";
			recorder.stop();
			video.pause();
		}
	});

	// close button leading back to normal video play back
	var close = recordCtrl.addChild('button');
	close.addClass("vjs-snippet-close");
	close.el().title = "close recording and return to video";
	close.on('click', function(){
		// switch back to normal player controls
		recordCtrl.hide();
		player.controlBar.show();
		player.el().focus();
	});
}

videojs.plugin('snippets', snippets);
