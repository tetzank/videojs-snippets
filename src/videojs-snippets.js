function snippets(options){
// 	"use strict";

	// globals
	var player = this;
	var video = player.el().querySelector('video');
	var dl_link;
	// hidden canvas for cropping and resizing
	var container, canvas, context;
	// globals for cropping and resizing
	var sx, sy, sw, sh;
	var dx, dy, dw, dh;
	var scale=1;
	var recorder;
	var recording=false; // recording flag, currently recording?
	var cropping=false;
	var mousing=false;

	// draw video to canvas
	// use requestAnimationFrame to render the video as often as possible
	var drawToCanvas = function(){
		// schedule next call to this function
		if(recording) requestAnimationFrame(drawToCanvas);
		// draw video data into the canvas
		context.drawImage(video, sx, sy, sw, sh, dx, dy, dw, dh);
	};

	function updateScale(){
		var rect = video.getBoundingClientRect();
		var scalew = canvas.el().width / rect.width;
		var scaleh = canvas.el().height / rect.height;
		scale = Math.max(Math.max(scalew, scaleh), 1);
		scale_txt.el().innerHTML = (Math.round(1/scale*100)/100) +"x";
	}

	// switch to recording
	// added to player object to be callable from outside, e.g. shortcut
	player.snippet = function(){
		player.pause();
		// loose keyboard focus
		player.el().blur();
		// switch control bar to recording controls
		player.controlBar.hide(); //FIXME: also disables progress bar, need copy or something for seeking
		recordCtrl.show();
		// display canvas
		parent.show();
		
		// crop & scale test
		// initially: everything
		sx=0; sy=0; sw=video.videoWidth; sh=video.videoHeight;
		dx=0; dy=0; dw=sw; dh=sh;
		// resize canvas
		canvas.el().width = dw;
		canvas.el().height = dh;
		// calculate scale
		updateScale();
		// still fit into player element
		var rect = video.getBoundingClientRect(); // use bounding rect instead of player.width/height because of fullscreen
		canvas.el().style.maxWidth  = rect.width  +"px";
		canvas.el().style.maxHeight = rect.height +"px";
		// draw video data into the canvas
		context.drawImage(video, sx, sy, sw, sh, dx, dy, dw, dh);
		//context.drawImage(video, 0, 0);

		// edit video in canvas, get stream of edited video
		var canvasStream = canvas.el().captureStream(15);

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
			// show download button
			dl_link.el().href = videoURL;
			dl_link.el().download = fname;
			dl_link.show();
		});
	};
	// icon on normal player control bar
	var snippet_btn = player.controlBar.addChild('button');
	snippet_btn.addClass("vjs-snippet-button");
	snippet_btn.el().title = "Switch to WebM recording";
	snippet_btn.on('click', player.snippet);

	// canvas stuff
	// add canvas parent container before draw control bar, so bar gets on top
	var parent = player.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl(null, {
				className: 'vjs-canvas-parent'
			}),
		})
	);
	parent.hide();
	container = parent.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl(null, {
				className: 'vjs-canvas-container'
			}),
		})
	);
	canvas = container.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl('canvas', {
			}),
		})
	);
	context = canvas.el().getContext("2d");


	//record control bar
	var recordCtrl = player.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl(null, {
				className: 'vjs-control-bar vjs-snippet-ctrl',
			}),
		})
	);
	recordCtrl.hide();

	//TODO: seeking: play button (just play, don't record) + progress bar
	//TODO: resize
	//TODO: firefox: cropping shifted
	//TODO: chromium: audio twice when recording

	// crop selection - only record selected area, crop rest
	var cropbox = container.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl('div', {
				innerHTML: "crop",
				className: 'vjs-canvas-cropbox'
			}),
		})
	);
	cropbox.hide();

	//FIXME: cropping from scaled down videos is shifted
	cropbox.on('mousedown', function(e){
		// update crop&scale values
		sx += scale * cropbox.el().offsetLeft  |0;
		sy += scale * cropbox.el().offsetTop   |0;
		sw  = scale * cropbox.el().offsetWidth |0;
		sh  = scale * cropbox.el().offsetHeight|0;
		dx = 0; dy = 0;
		dw=sw/**scale*/ |0; dh=sh/**scale*/ |0;
		// update size of canvas
		canvas.el().width = dw;
		canvas.el().height = dh;
		// recalculate scale
		updateScale();
		// draw video data into the canvas
		context.drawImage(video, sx, sy, sw, sh, dx, dy, dw, dh);

		cropbox.hide();
		e.stopPropagation(); //otherwise container gets mousedown as well
	});

	container.on('mousedown', function(e){
		cropbox.show();
		var pos = player.el().getBoundingClientRect();
		var x = e.clientX - pos.left;
		var y = e.clientY - pos.top;

		cropbox.el().style.width = 0;
		cropbox.el().style.height = 0;
		cropbox.el().style.left = x + "px";
		cropbox.el().style.top = y + "px";

		mousing = true;
	});
	container.on('mousemove', function(e){
		if(mousing){
			var pos = container.el().getBoundingClientRect();
			var x = e.clientX - pos.left;
			var y = e.clientY - pos.top;

			cropbox.el().style.width = (x - cropbox.el().offsetLeft) + "px";
			cropbox.el().style.height = (y - cropbox.el().offsetTop) + "px";
			e.preventDefault();
		}
	});
	container.on('mouseup', function(){
		mousing = false;
	});
	container.on('mouseleave', function(){
		mousing = false;
	});
	
	// start/stop recording
	var clap = recordCtrl.addChild('button');
	clap.addClass("vjs-snippet-clap");
	clap.el().title = "start/stop recording";
	clap.on('click', function(){
		recording = !recording;
		if(recording){
			// start the animation loop
			requestAnimationFrame(drawToCanvas);
			// indicate recording
			clap.el().style.color = "red";
			//this.innerHTML = "stop recording";
			// start recording
			recorder.start();
			// start video
			video.play();
		}else{
			// indicate stopped recording
			clap.el().style.color = "inherit";
			//this.innerHTML = "start recording";
			recorder.stop();
			video.pause();
		}
	});

	dl_link = recordCtrl.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl('a', {
				className: 'vjs-control vjs-snippet-download',
				title: 'Download',
				style: 'color: inherit'
			})
		})
	);
	dl_link.hide();

	// close button leading back to normal video play back
	var close = recordCtrl.addChild('button');
	close.addClass("vjs-snippet-close");
	close.el().title = "close recording and return to video";
	close.on('click', function(){
		// switch back to normal player controls
		recordCtrl.hide();
		dl_link.hide();
		// hide all canvas stuff
		parent.hide();
		// switch back to normal player controls
		player.controlBar.show();
		player.el().focus();
	});

	// scale display
	var scale_txt = recordCtrl.addChild(
		new videojs.Component(player, {
			el: videojs.Component.prototype.createEl(null, {
				className: 'vjs-scale', innerHTML: '1', title: 'scale factor'
			}),
		})
	);
}

videojs.plugin('snippets', snippets);
