/*
  Overwolf NPAPI Plugin
  Copyright (c) 2014 Overwolf Ltd.
*/
#ifndef NNSSCRIPTABLEOBJECTNPDETECTDOTNETPLUGIN_H_
#define NNSSCRIPTABLEOBJECTNPDETECTDOTNETPLUGIN_H_

#include "nsScriptableObjectBase.h"
#include <map>

namespace utils {
class Thread; // forward declaration
}

class nsScriptableObjectnpDetectDotNetPlugin : public nsScriptableObjectBase {
public:
  nsScriptableObjectnpDetectDotNetPlugin(NPP npp);
  virtual ~nsScriptableObjectnpDetectDotNetPlugin(void);

public:
  bool Init();

// nsScriptableObjectBase overrides
public:
  virtual bool HasMethod(NPIdentifier name);
  virtual bool Invoke(
    NPIdentifier name, 
    const NPVariant *args, 
    uint32_t argCount, 
    NPVariant *result);
  virtual bool HasProperty(NPIdentifier name);
  virtual bool GetProperty(NPIdentifier name, NPVariant *result);
  virtual bool SetProperty(NPIdentifier name, const NPVariant *value);

// public methods
private:
  // Because we want to have a responsive plugin - we make sure that all
  // our functions run on a separate thread (not on the main browser
  // thread).  To achieve this, we create a Task function (see Thread.h) that 
  // runs on a separate thread triggers a callback

  // getDotNet("version", function(status) { console.log(status) });
  bool GetDotNet(
	  NPIdentifier name,
	  const NPVariant *args,
	  uint32_t argCount,
	  NPVariant *result);

// These are the functions that run on a separate thread - 
// there is one for each public function we expose.
// Eventually, they will run TriggerCallbackOnMainThread
private:
  void GetDotNetTask(
	  const std::string& version,
	  NPObject* callback);

// member variables
private:
  // defines a generic method
  typedef bool (nsScriptableObjectnpDetectDotNetPlugin::*MethodHandle)(
    NPIdentifier,
    const NPVariant*, 
    uint32_t, 
    NPVariant*);

  // holds the public methods
  typedef std::map<NPIdentifier, MethodHandle> MethodsMap;
  MethodsMap methods_;

  // holds the public methods
  typedef std::map<NPIdentifier, std::string> PropertiesMap;
  PropertiesMap properties_;

  // good idea for when having an autonomous thread sending callbacks
  bool shutting_down_;

  // this allows us to run our code on a separate thread than the 
  // main browser thread - to be more responsive
  std::auto_ptr<utils::Thread> thread_;
};

// declare our NPObject-derived scriptable object class
DECLARE_NPOBJECT_CLASS_WITH_BASE(
  nsScriptableObjectnpDetectDotNetPlugin,
  AllocateNpObject<nsScriptableObjectnpDetectDotNetPlugin>);


#endif // NNSSCRIPTABLEOBJECTNPDETECTDOTNETPLUGIN_H_
