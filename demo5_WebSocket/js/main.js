/*
 *  Copyright (c) 2015 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree.
 */

'use strict';

const callButton = document.getElementById('callButton');

callButton.disabled = true;

callButton.addEventListener('click', call);

const localVideo = document.getElementById('localVideo');

const startButton = document.getElementById('startButton');


let localStream;
let pc1;
let websocket;
let num = 1;

//开启音视频源
async function start() {
  console.log('Requesting local stream'); 
  try {
	  //捕获桌面的视频流，放到localVideo中
    const stream = await navigator.mediaDevices.getDisplayMedia({audio: false, video: true});
    console.log('Received local stream');
    localVideo.srcObject = stream;
    localStream = stream;
    callButton.disabled = false;
  } catch (e) {
    alert(`getDisplayMedia() error: ${e.name}`);
  }
}

//拨打，建立连接
async function call() {

  callButton.disabled = true;

  const configuration = {
    sdpSemantics: "unified-plan",
    enableDtlsSrtp: true,
  };
  //源连接，
  pc1 = new RTCPeerConnection(configuration);
  //当ice准备好后，加到目标源中
  pc1.addEventListener('icecandidate', e => onIceCandidate(pc1, e));
  //把localStream的音视频，放到源中
  localStream.getTracks().forEach(track => pc1.addTrack(track, localStream));

  try {
    console.log('pc1 createOffer start');
	
	const offerOptions = {
		// offerToReceiveAudio: 1,
		offerToReceiveVideo: 1
	};

	//创建和设置连接描述
    const desc_pc1 = await pc1.createOffer(offerOptions);
	console.log("desc_pc1:");
	console.log(desc_pc1);
  console.log("desc_pc1 finish");

	//发送sdp
	const req = {
          type: "sdp",
          content: desc_pc1,
      };
	websocket.send(JSON.stringify(req));

	await pc1.setLocalDescription(desc_pc1);
	
  } catch (e) {
    onCreateSessionDescriptionError(e);
  }
}

async function onIceCandidate(pc, event) {


  try {
      if (num != 1){
        return;
      }
      num = num + 1;
	  console.log(event.candidate);

	  //发送ice
      const req = {
        type: "candidate",
        content: event.candidate,
      };
      websocket.send(JSON.stringify(req));
      console.log("send ice");

  } catch (e) {
    onAddIceCandidateError(pc, e);
  }
  //console.log(`${getName(pc)} ICE candidate:\n${event.candidate ? event.candidate.candidate : '(null)'}`);
}

function initChat() {
  websocket.addEventListener("open", () => {
  });
}

function receiveWebsocketMessage() {
  websocket.addEventListener("message", messageHander);
}

async function messageHander(data){
  // console.log("get message is : ");
  // console.log(data.data);
  const event = JSON.parse(data.data);
  console.log("message:" + event['type']);

  switch (event['type']) {
    case "answer_sdp":
      console.log("get answer_sdp:");
      const desc_pc2 = event['content'];
      console.log(desc_pc2);
      await pc1.setRemoteDescription(new RTCSessionDescription(desc_pc2));
      break;

    case "answer_candidate":
      console.log("get answer_ice:");
      console.log(event['type']);
      console.log(event['content']);
      try {
        pc1.addIceCandidate(new RTCIceCandidate(event['content']));
      } catch (error) {
        console.error('Error adding ICE candidate:', error);
      }
      break;
  }
}


window.addEventListener("load", () => {
  // Open the WebSocket connection and register event handlers.

  websocket = new WebSocket("wss://172.16.41.98:20080");
  // websocket = new WebSocket("ws://localhost:20080/");
  initChat();
  receiveWebsocketMessage();
  startButton.addEventListener('click', async () => {
    start();
  });
  
});


function onCreateSessionDescriptionError(error) {
  console.log(`Failed to create session description: ${error.toString()}`);
}


function onSetLocalSuccess(pc) {
  console.log(` setLocalDescription complete`);
}

function onSetRemoteSuccess(pc) {
  console.log(` setRemoteDescription complete`);
}

function onSetSessionDescriptionError(error) {
  console.log(`Failed to set session description: ${error.toString()}`);
}


function onAddIceCandidateSuccess(pc) {
  console.log(` addIceCandidate success`);
}

function onAddIceCandidateError(pc, error) {
  console.log(` failed to add ICE Candidate: ${error.toString()}`);
}

