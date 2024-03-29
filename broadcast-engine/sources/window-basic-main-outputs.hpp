#pragma once

#include <string>

class SourceManager;

struct BasicOutputHandler {
	OBSOutput              fileOutput;
	OBSOutput              streamOutput;
	OBSOutput              replayBuffer;
	bool                   streamingActive = false;
	bool                   recordingActive = false;
	bool                   delayActive = false;
	bool                   replayBufferActive = false;
	SourceManager           *main;

	std::string            outputType;
	std::string            lastError;

	OBSSignal              startRecording;
	OBSSignal              stopRecording;
	OBSSignal              startReplayBuffer;
	OBSSignal              stopReplayBuffer;
	OBSSignal              startStreaming;
	OBSSignal              stopStreaming;
	OBSSignal              streamDelayStarting;
	OBSSignal              streamStopping;
	OBSSignal              recordStopping;
	OBSSignal              replayBufferStopping;
	// extend by wyb for obs sdk, reconnect callback
	OBSSignal			   reconnectStreaming;
	OBSSignal			   reconnectSuccessStreaming;

	inline BasicOutputHandler(SourceManager *main_) : main(main_) {}

	virtual ~BasicOutputHandler() {};

	virtual bool StartStreaming(obs_service_t *service) = 0;
	virtual bool StartRecording() = 0;
	virtual bool StartReplayBuffer() {return false;}
	virtual void StopStreaming(bool force = false) = 0;
	virtual void StopRecording(bool force = false) = 0;
	virtual void StopReplayBuffer(bool force = false) {(void)force;}
	virtual bool StreamingActive() const = 0;
	virtual bool RecordingActive() const = 0;
	virtual bool ReplayBufferActive() const {return false;}

	virtual void Update() = 0;

	inline bool Active() const
	{
		return streamingActive || recordingActive || delayActive ||
			replayBufferActive;
	}
};

BasicOutputHandler *CreateSimpleOutputHandler(SourceManager *main);
BasicOutputHandler *CreateAdvancedOutputHandler(SourceManager *main);
