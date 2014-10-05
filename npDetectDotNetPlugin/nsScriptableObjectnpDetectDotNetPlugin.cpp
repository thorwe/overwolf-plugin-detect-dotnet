/*
  Overwolf NPAPI Plugin
  Copyright (c) 2014 Overwolf Ltd.
*/
#include "nsScriptableObjectnpDetectDotNetPlugin.h"
#include "utils/Thread.h"
#include "utils/DetectDotNet.h"

#include <sstream>

#define REGISTER_METHOD(name, method) { \
  methods_[NPN_GetStringIdentifier(name)] = &method; \
}

#define REGISTER_GET_PROPERTY(name, val) { \
  properties_[NPN_GetStringIdentifier(name)] = val; \
}

nsScriptableObjectnpDetectDotNetPlugin::nsScriptableObjectnpDetectDotNetPlugin(NPP npp) :
  nsScriptableObjectBase(npp),
  shutting_down_(false) {
}

nsScriptableObjectnpDetectDotNetPlugin::~nsScriptableObjectnpDetectDotNetPlugin(void) {
  shutting_down_ = true;
  
  if (thread_.get()) {
    thread_->Stop();
  }
}

bool nsScriptableObjectnpDetectDotNetPlugin::Init() {
  REGISTER_METHOD(
	  "checkDotNet",
	  nsScriptableObjectnpDetectDotNetPlugin::GetDotNet);

  thread_.reset(new utils::Thread());
  return thread_->Start();
}

bool nsScriptableObjectnpDetectDotNetPlugin::HasMethod(NPIdentifier name) {
#ifdef _DEBUG
  NPUTF8* name_utf8 = NPN_UTF8FromIdentifier(name);
  NPN_MemFree((void*)name_utf8);
#endif

  // does the method exist?
  return (methods_.find(name) != methods_.end());
}

bool nsScriptableObjectnpDetectDotNetPlugin::Invoke(
  NPIdentifier name, 
  const NPVariant *args, 
  uint32_t argCount, 
  NPVariant *result) {
#ifdef _DEBUG
      NPUTF8* szName = NPN_UTF8FromIdentifier(name);
      NPN_MemFree((void*)szName);
#endif

  // dispatch method to appropriate handler
  MethodsMap::iterator iter = methods_.find(name);
  
  if (iter == methods_.end()) {
    // should never reach here
    NPN_SetException(this, "bad function called??");
    return false;
  }

  return (this->*iter->second)(name, args, argCount, result);
}

/************************************************************************/
/* Public properties
/************************************************************************/
bool nsScriptableObjectnpDetectDotNetPlugin::HasProperty(NPIdentifier name) {
#ifdef _DEBUG
  NPUTF8* name_utf8 = NPN_UTF8FromIdentifier(name);
  NPN_MemFree((void*)name_utf8);
#endif

  // does the property exist?
  return (properties_.find(name) != properties_.end());
}

bool nsScriptableObjectnpDetectDotNetPlugin::GetProperty(
  NPIdentifier name, NPVariant *result) {

  PropertiesMap::iterator iter = properties_.find(name);
  if (iter == properties_.end()) {
    NPN_SetException(this, "unknown property!?");
    return true;
  }

  char *resultString = (char*)NPN_MemAlloc(iter->second.size());
  memcpy(
    resultString, 
    iter->second.c_str(), 
    iter->second.size());

  STRINGN_TO_NPVARIANT(resultString, iter->second.size(), *result);

  return true;
}

bool nsScriptableObjectnpDetectDotNetPlugin::SetProperty(
  NPIdentifier name, const NPVariant *value) {
  NPN_SetException(this, "this property is read-only!");
  return true;
}


/************************************************************************/
/* Public methods
/************************************************************************/

bool nsScriptableObjectnpDetectDotNetPlugin::GetDotNet(
	NPIdentifier name,
	const NPVariant *args,
	uint32_t argCount,
	NPVariant *result){
	
	if (argCount > 2 || argCount == 0 ||
		!NPVARIANT_IS_OBJECT(args[argCount-1]) ||
		 ((argCount == 2) && !NPVARIANT_IS_STRING(args[0]))) {
		NPN_SetException(this, "invalid params passed to function");
		return true;
	}

	// add ref count to callback object so it won't delete
	NPN_RetainObject(NPVARIANT_TO_OBJECT(args[argCount-1]));

	// convert into std::string
	std::string arg = "";
	if (argCount > 1) {
		arg.append(
			NPVARIANT_TO_STRING(args[0]).UTF8Characters,
			NPVARIANT_TO_STRING(args[0]).UTF8Length);
	}

	// post to separate thread so that we are responsive
	return thread_->PostTask(
		std::bind(
		&nsScriptableObjectnpDetectDotNetPlugin::GetDotNetTask,
		this,
		arg,
		NPVARIANT_TO_OBJECT(args[argCount-1])));
}


/************************************************************************/
/* Separate thread implementations for public functions
/************************************************************************/

void nsScriptableObjectnpDetectDotNetPlugin::GetDotNetTask(
	const std::string& version,
	NPObject* callback) {

	if (shutting_down_) {
		return;
	}

	int val = -1;
	if (version.length() == 0) {
		val = utils::CheckDotNet();
	}
	else if (version == "4.5.2") {
		val = (int)utils::IsNetfx452Installed();
	}
	else if (version == "4.5.1") {
		val = (int)utils::IsNetfx451Installed();
	}
	else if (version == "4.5") {
		val = (int)utils::IsNetfx45Installed();
	}
	else if (version == "4.0") {
		val = (int)utils::IsNetfx40FullInstalled();
	}
	else if (version == "4.0C") {
		val = (int)utils::IsNetfx40ClientInstalled();
	}
	else if (version == "3.5") {
		val = (int)(utils::IsNetfx20Installed() && utils::IsNetfx30Installed() && utils::IsNetfx35Installed());
	}
	else if (version == "3.0") {
		val = (int)(utils::IsNetfx20Installed() && utils::IsNetfx30Installed());
	}
	else if (version == "2.0") {
		val = (int)utils::IsNetfx20Installed();
	}
	else if (version == "1.1") {
		val = (int)utils::IsNetfx11Installed();
	}
	else if (version == "1.0") {
		val = (int)utils::IsNetfx10Installed();
	}

	NPVariant arg;
	NPVariant ret_val;

	INT32_TO_NPVARIANT(
		val,
		arg);

	// fire callback
	NPN_InvokeDefault(
		__super::npp_,
		callback,
		&arg,
		1,
		&ret_val);

	NPN_ReleaseVariantValue(&ret_val);
}
