#pragma once
#include <fstream>
#include <unordered_map>
#include "IBroadcastEngine.h"
#include "obs.hpp"
#include "window-basic-main-outputs.hpp"
#include "util/util.hpp"


struct AddSourceData
{
	obs_source_t *source;
	bool visible;

};

class SourceManager;
class SceneManager;
//struct BasicOutputHandler;
class BroadcastEngineImpl :public IBroadcastEngine
{
public:
	BroadcastEngineImpl(IBroadcastEngineObserver *);
	virtual ~BroadcastEngineImpl();
public:
	static void do_log(int log_level, const char *msg, va_list args, void *param);
	void  logSink(const std::string &msg, int log_level);

public:
//IBroadcastEngine impl
//��ʼ��SDK
	virtual int init()override;
	// ����/���� ����Դ�ļ�  
	virtual int load() override;
	virtual int save() override;

	//���/ɾ�� ����Դ
	virtual int addSource(Broadcast::ESourceType type,const char* srcName, const char *scene_name) override;
	virtual int removeSource(const char *srcName) override;

	//�ж�ĳ��/ĳ������ ����Դ�Ƿ���� 
	virtual bool hasSource(const char *srcName) override;
	virtual bool hasSource(Broadcast::ESourceType type)override;

	virtual Broadcast::ESourceType getSourceTypeByName(const char* srcName) override;
	virtual void enumSource(std::function<bool(Broadcast::ESourceType type, const char *src_name)> callback)override;
	//�ٿ�Դ�ڳ����е���ȾЧ��
	virtual int manipulateSource(const char *srcName, Broadcast::ESourceActiton operation) override;
	//��ʼ/ֹͣԤ�� ������Ƶ����ԴԤ��
	virtual bool startPreviewSource(const char *srcName, const void* Window) override;
	virtual bool stopPreviewSource(const char *srcName) override;

	// ��ó���Դ������ Ԥ��������ؽӿ�
	virtual std::shared_ptr<ISceneManager> sceneManager()override;

	// ������ؽӿ�
	int startStream(const char *url, const char* key) override;
	int stopStream(bool bForce)override;
	virtual int getOutputSize(int &width, int &height) override;
	virtual int setOutputSize(int width, int height) override;

	/**************** ¼����ؽӿ� ******************/
	// ��ȡ/���� ¼���ļ�����·��
	virtual int setRecordFilePath(const char *file_path) override;
	virtual const char* getRecordFilePath() override;

	// ��ȡ/���� ¼���ļ���ʽ
	virtual int setRecordFileFormat(const char *recFmt) override;
	virtual const char * getRecordFileFormat() override;

	// ��ʼ/���� ¼��
	virtual int startRecord() override;
	virtual int stopRecord() override;

public:
	IBroadcastEngineObserver *engineObserver() 
	{ 
		return m_pEngineObserver; 
	}
	ConfigFile& Config() 
	{ 
		return m_globalConfigFile; 
	}

	int lookupByName(const std::string & name) 
	{ 
		return m_lookupByName[name];
	}

	std::string lookupByType(int type)
	{
		return m_lookupByType[type];
	}

	std::shared_ptr<SourceManager> sourceManger() 
	{
		return m_sourceManager; 
	}
	std::shared_ptr<SceneManager> scene_manager();
	OBSData getBaseData() { return m_baseDATA; }
protected:
	int createAudioInputSource(const char *src_name);
	int createAudioOutputSource(const char *src_name);
private:
	bool initGlobalConfig();
	std::string getSettingFilePath(const std::string &file_name);
	bool  loadSourceFromSetting(const std::string &file_name);
	bool  saveSourceToSetting(size_t flag);
	Broadcast::ESourceType getSourceType(const char *id);
private:
	IBroadcastEngineObserver *				m_pEngineObserver = nullptr;
	std::shared_ptr<ISceneManager>			m_sceneManager;
	std::shared_ptr<SourceManager>			m_sourceManager;
	ConfigFile								m_globalConfigFile;
	std::unordered_map<int, std::string>	m_lookupByType;
	std::unordered_map<std::string, int>	m_lookupByName;
	OBSData									m_baseDATA;
	std::unique_ptr<BasicOutputHandler>		m_outputHandler;
};

extern  BroadcastEngineImpl * g_pBroadcastEngine;