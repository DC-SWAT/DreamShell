/*
   AngelCode Scripting Library
   Copyright (c) 2003-2011 Andreas Jonsson

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any
   damages arising from the use of this software.

   Permission is granted to anyone to use this software for any
   purpose, including commercial applications, and to alter it and
   redistribute it freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you
      must not claim that you wrote the original software. If you use
      this software in a product, an acknowledgment in the product
      documentation would be appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and
      must not be misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
      distribution.

   The original version of this library can be located at:
   http://www.angelcode.com/angelscript/

   Andreas Jonsson
   andreas@angelcode.com
*/


//
// as_c.cpp
//
// A C interface to the library 
//

// Include the C++ interface header so we can call the proper methods
#include "../../angelscript/include/angelscript.h"

BEGIN_AS_NAMESPACE

typedef void (*asBINARYREADFUNC_t)(void *ptr, asUINT size, void *param);
typedef void (*asBINARYWRITEFUNC_t)(const void *ptr, asUINT size, void *param);

typedef enum { asTRUE = 1, asFALSE = 0 } asBOOL;

class asCBinaryStreamC : public asIBinaryStream
{
public:
	asCBinaryStreamC(asBINARYWRITEFUNC_t write, asBINARYREADFUNC_t read, void *param) {this->write = write; this->read = read; this->param = param;}

	void Write(const void *ptr, asUINT size) { write(ptr, size, param); }
	void Read(void *ptr, asUINT size) { read(ptr, size, param); }

	asBINARYREADFUNC_t read;
	asBINARYWRITEFUNC_t write;
	void *param;
};

extern "C"
{

	// Implement a global wrapper function for each of the library's interface methods

	///////////////////////////////////////////
	// asIScriptEngine
	
	// Memory management
	AS_API int                asEngine_AddRef(asIScriptEngine *e)                                                                                                                                      { return e->AddRef(); }
	AS_API int                asEngine_Release(asIScriptEngine *e)                                                                                                                                     { return e->Release(); }

	// Engine properties
	AS_API int                asEngine_SetEngineProperty(asIScriptEngine *e, asEEngineProp property, asPWORD value)                                                                                    { return e->SetEngineProperty(property, value); }
	AS_API asPWORD            asEngine_GetEngineProperty(asIScriptEngine *e, asEEngineProp property)                                                                                                   { return e->GetEngineProperty(property); }

	// Compiler messages
	AS_API int                asEngine_SetMessageCallback(asIScriptEngine *e, asFUNCTION_t callback, void *obj, asDWORD callConv)                                                                      { return e->SetMessageCallback(asFUNCTION(callback), obj, callConv); }
	AS_API int                asEngine_ClearMessageCallback(asIScriptEngine *e)                                                                                                                        { return e->ClearMessageCallback(); }
	AS_API int                asEngine_WriteMessage(asIScriptEngine *e, const char *section, int row, int col, asEMsgType type, const char *message)                                                   { return e->WriteMessage(section, row, col, type, message); }

	// JIT Compiler
	AS_API int                asEngine_SetJITCompiler(asIScriptEngine *e, asIJITCompiler *compiler) { return e->SetJITCompiler(compiler); }
	AS_API asIJITCompiler *   asEngine_GetJITCompiler(asIScriptEngine *e) { return e->GetJITCompiler(); }

	// Global functions
	AS_API int                asEngine_RegisterGlobalFunction(asIScriptEngine *e, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)                                                 { return e->RegisterGlobalFunction(declaration, asFUNCTION(funcPointer), callConv); }
	AS_API int                asEngine_GetGlobalFunctionCount(asIScriptEngine *e)                                                                                                                      { return e->GetGlobalFunctionCount(); }
	AS_API int                asEngine_GetGlobalFunctionIdByIndex(asIScriptEngine *e, asUINT index)                                                                                                    { return e->GetGlobalFunctionIdByIndex(index); }

	// Global properties
	AS_API int                asEngine_RegisterGlobalProperty(asIScriptEngine *e, const char *declaration, void *pointer)                                                                              { return e->RegisterGlobalProperty(declaration, pointer); }
	AS_API int                asEngine_GetGlobalPropertyCount(asIScriptEngine *e)                                                                                                                      { return e->GetGlobalPropertyCount(); }
	AS_API int                asEngine_GetGlobalPropertyByIndex(asIScriptEngine *e, asUINT index, const char **name, int *typeId, asBOOL *isConst, const char **configGroup, void **pointer)           { bool _isConst; int r = e->GetGlobalPropertyByIndex(index, name, typeId, &_isConst, configGroup, pointer); if( isConst ) *isConst = (_isConst ? asTRUE : asFALSE); return r; }

	// Object types
	AS_API int                asEngine_RegisterObjectType(asIScriptEngine *e, const char *name, int byteSize, asDWORD flags)                                                                           { return e->RegisterObjectType(name, byteSize, flags); }
	AS_API int                asEngine_RegisterObjectProperty(asIScriptEngine *e, const char *obj, const char *declaration, int byteOffset)                                                            { return e->RegisterObjectProperty(obj, declaration, byteOffset); }
	AS_API int                asEngine_RegisterObjectMethod(asIScriptEngine *e, const char *obj, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv)                                  { return e->RegisterObjectMethod(obj, declaration, asFUNCTION(funcPointer), callConv); }
	AS_API int                asEngine_RegisterObjectBehaviour(asIScriptEngine *e, const char *datatype, asEBehaviours behaviour, const char *declaration, asFUNCTION_t funcPointer, asDWORD callConv) { return e->RegisterObjectBehaviour(datatype, behaviour, declaration, asFUNCTION(funcPointer), callConv); }
	AS_API int                asEngine_RegisterInterface(asIScriptEngine *e, const char *name)                                                                                                         { return e->RegisterInterface(name); }
	AS_API int                asEngine_RegisterInterfaceMethod(asIScriptEngine *e, const char *intf, const char *declaration)                                                                          { return e->RegisterInterfaceMethod(intf, declaration); }
	AS_API asIObjectType *    asEngine_GetObjectTypeByIndex(asIScriptEngine *e, asUINT index)                                                                                                          { return e->GetObjectTypeByIndex(index); }
	AS_API int                asEngine_GetObjectTypeCount(asIScriptEngine *e)                                                                                                                          { return e->GetObjectTypeCount(); }

	// String factory
	AS_API int                asEngine_RegisterStringFactory(asIScriptEngine *e, const char *datatype, asFUNCTION_t factoryFunc, asDWORD callConv)                                                     { return e->RegisterStringFactory(datatype, asFUNCTION(factoryFunc), callConv); }
	AS_API int                asEngine_GetStringFactoryReturnTypeId(asIScriptEngine *e)                                                                                                                { return e->GetStringFactoryReturnTypeId(); }

	// Default array type
	AS_API int                asEngine_RegisterDefaultArrayType(asIScriptEngine *e, const char *type) { return e->RegisterDefaultArrayType(type); }
	AS_API int                asEngine_GetDefaultArrayTypeId(asIScriptEngine *e) { return e->GetDefaultArrayTypeId(); }

	// Enums
	AS_API int                asEngine_RegisterEnum(asIScriptEngine *e, const char *type)                                                                                                              { return e->RegisterEnum(type); }
	AS_API int                asEngine_RegisterEnumValue(asIScriptEngine *e, const char *type, const char *name, int value)                                                                            { return e->RegisterEnumValue(type,name,value); }
	AS_API int                asEngine_GetEnumCount(asIScriptEngine *e)                                                                                                                                { return e->GetEnumCount(); }
	AS_API const char *       asEngine_GetEnumByIndex(asIScriptEngine *e, asUINT index, int *enumTypeId, const char **configGroup)                                                                     { return e->GetEnumByIndex(index, enumTypeId, configGroup); }
	AS_API int                asEngine_GetEnumValueCount(asIScriptEngine *e, int enumTypeId)                                                                                                           { return e->GetEnumValueCount(enumTypeId); }
	AS_API const char *       asEngine_GetEnumValueByIndex(asIScriptEngine *e, int enumTypeId, asUINT index, int *outValue)                                                                            { return e->GetEnumValueByIndex(enumTypeId, index, outValue); }

	// Funcdefs
	AS_API int                asEngine_RegisterFuncdef(asIScriptEngine *e, const char *decl) { return e->RegisterFuncdef(decl); }
	AS_API asUINT             asEngine_GetFuncdefCount(asIScriptEngine *e) { return e->GetFuncdefCount(); }
	AS_API asIScriptFunction *asEngine_GetFuncdefByIndex(asIScriptEngine *e, asUINT index, const char **configGroup) { return e->GetFuncdefByIndex(index, configGroup); }

	// Typedefs
	AS_API int                asEngine_RegisterTypedef(asIScriptEngine *e, const char *type, const char *decl)                                                                                         { return e->RegisterTypedef(type,decl); }
	AS_API int                asEngine_GetTypedefCount(asIScriptEngine *e)                                                                                                                             { return e->GetTypedefCount(); }
	AS_API const char *       asEngine_GetTypedefByIndex(asIScriptEngine *e, asUINT index, int *typeId, const char **configGroup)                                                                      { return e->GetTypedefByIndex(index, typeId, configGroup); }

	// Configuration groups
	AS_API int                asEngine_BeginConfigGroup(asIScriptEngine *e, const char *groupName)                                                                                                     { return e->BeginConfigGroup(groupName); }
	AS_API int                asEngine_EndConfigGroup(asIScriptEngine *e)                                                                                                                              { return e->EndConfigGroup(); }
	AS_API int                asEngine_RemoveConfigGroup(asIScriptEngine *e, const char *groupName)                                                                                                    { return e->RemoveConfigGroup(groupName); }
	AS_API int                asEngine_SetConfigGroupModuleAccess(asIScriptEngine *e, const char *groupName, const char *module, asBOOL haveAccess)                                                    { return e->SetConfigGroupModuleAccess(groupName, module, haveAccess ? true : false); }

	// Script modules
	AS_API asIScriptModule *  asEngine_GetModule(asIScriptEngine *e, const char *module, asEGMFlags flag)                                                                                              { return e->GetModule(module, flag); }
	AS_API int                asEngine_DiscardModule(asIScriptEngine *e, const char *module)                                                                                                           { return e->DiscardModule(module); }

	// Script functions
	AS_API asIScriptFunction *asEngine_GetFunctionDescriptorById(asIScriptEngine *e, int funcId)                                                                                                       { return e->GetFunctionDescriptorById(funcId); }

	// Type identification
	AS_API asIObjectType *    asEngine_GetObjectTypeById(asIScriptEngine *e, int typeId)                                                                                                               { return e->GetObjectTypeById(typeId); }
	AS_API int                asEngine_GetTypeIdByDecl(asIScriptEngine *e, const char *decl)                                                                                                           { return e->GetTypeIdByDecl(decl); }
	AS_API const char *       asEngine_GetTypeDeclaration(asIScriptEngine *e, int typeId)                                                                                                              { return e->GetTypeDeclaration(typeId); }
	AS_API int                asEngine_GetSizeOfPrimitiveType(asIScriptEngine *e, int typeId)                                                                                                          { return e->GetSizeOfPrimitiveType(typeId); }

	// Script execution
	AS_API asIScriptContext * asEngine_CreateContext(asIScriptEngine *e)                                                                                                                               { return e->CreateContext(); }
	AS_API void *             asEngine_CreateScriptObject(asIScriptEngine *e, int typeId)                                                                                                              { return e->CreateScriptObject(typeId); }
	AS_API void *             asEngine_CreateScriptObjectCopy(asIScriptEngine *e, void *obj, int typeId)                                                                                               { return e->CreateScriptObjectCopy(obj, typeId); }
	AS_API void               asEngine_CopyScriptObject(asIScriptEngine *e, void *dstObj, void *srcObj, int typeId)                                                                                    { e->CopyScriptObject(dstObj, srcObj, typeId); }
	AS_API void               asEngine_ReleaseScriptObject(asIScriptEngine *e, void *obj, int typeId)                                                                                                  { e->ReleaseScriptObject(obj, typeId); }
	AS_API void               asEngine_AddRefScriptObject(asIScriptEngine *e, void *obj, int typeId)                                                                                                   { e->AddRefScriptObject(obj, typeId); }
	AS_API asBOOL             asEngine_IsHandleCompatibleWithObject(asIScriptEngine *e, void *obj, int objTypeId, int handleTypeId)                                                                    { return e->IsHandleCompatibleWithObject(obj, objTypeId, handleTypeId) ? asTRUE : asFALSE; }

	// String interpretation
	AS_API asETokenClass      asEngine_ParseToken(asIScriptEngine *e, const char *string, size_t stringLength, int *tokenLength) { return e->ParseToken(string, stringLength, tokenLength); }

	// Garbage collection
	AS_API int                asEngine_GarbageCollect(asIScriptEngine *e, asDWORD flags)                                                                                                               { return e->GarbageCollect(flags); }
	AS_API void               asEngine_GetGCStatistics(asIScriptEngine *e, asUINT *currentSize, asUINT *totalDestroyed, asUINT *totalDetected)                                                         { e->GetGCStatistics(currentSize, totalDestroyed, totalDetected); }
	AS_API void               asEngine_NotifyGarbageCollectorOfNewObject(asIScriptEngine *e, void *obj, int typeId)                                                                                    { e->NotifyGarbageCollectorOfNewObject(obj, typeId); }
	AS_API void               asEngine_GCEnumCallback(asIScriptEngine *e, void *obj)                                                                                                                   { e->GCEnumCallback(obj); }

	// User data
	AS_API void *             asEngine_SetUserData(asIScriptEngine *e, void *data)                                                                                                                     { return e->SetUserData(data); }
	AS_API void *             asEngine_GetUserData(asIScriptEngine *e)                                                                                                                                 { return e->GetUserData(); }
	AS_API void               asEngine_SetEngineUserDataCleanupCallback(asIScriptEngine *e, asCLEANENGINEFUNC_t callback) { e->SetEngineUserDataCleanupCallback(callback); }
	AS_API void               asEngine_SetContextUserDataCleanupCallback(asIScriptEngine *e, asCLEANCONTEXTFUNC_t callback) { e->SetContextUserDataCleanupCallback(callback); }
	AS_API void               asEngine_SetFunctionUserDataCleanupCallback(asIScriptEngine *e, asCLEANFUNCTIONFUNC_t callback) { e->SetFunctionUserDataCleanupCallback(callback); }

	///////////////////////////////////////////
	// asIScriptModule

	AS_API asIScriptEngine   *asModule_GetEngine(asIScriptModule *m)                                                                               { return m->GetEngine(); }
	AS_API void               asModule_SetName(asIScriptModule *m, const char *name)                                                               { m->SetName(name); }
	AS_API const char        *asModule_GetName(asIScriptModule *m)                                                                                 { return m->GetName(); }

	// Compilation
	AS_API int                asModule_AddScriptSection(asIScriptModule *m, const char *name, const char *code, size_t codeLength, int lineOffset) { return m->AddScriptSection(name, code, codeLength, lineOffset); }
	AS_API int                asModule_Build(asIScriptModule *m)                                                                                   { return m->Build(); }
	AS_API int                asModule_CompileFunction(asIScriptModule *m, const char *sectionName, const char *code, int lineOffset, asDWORD compileFlags, asIScriptFunction **outFunc) { return m->CompileFunction(sectionName, code, lineOffset, compileFlags, outFunc); }
	AS_API int                asModule_CompileGlobalVar(asIScriptModule *m, const char *sectionName, const char *code, int lineOffset) { return m->CompileGlobalVar(sectionName, code, lineOffset); }

	// Functions
	AS_API asUINT             asModule_GetFunctionCount(asIScriptModule *m)                                                                        { return m->GetFunctionCount(); }
	AS_API int                asModule_GetFunctionIdByIndex(asIScriptModule *m, int index)                                                         { return m->GetFunctionIdByIndex(index); }
	AS_API int                asModule_GetFunctionIdByName(asIScriptModule *m, const char *name)                                                   { return m->GetFunctionIdByName(name); }
	AS_API int                asModule_GetFunctionIdByDecl(asIScriptModule *m, const char *decl)                                                   { return m->GetFunctionIdByDecl(decl); }
	AS_API asIScriptFunction *asModule_GetFunctionDescriptorByIndex(asIScriptModule *m, int index)                                                 { return m->GetFunctionDescriptorByIndex(index); }
	AS_API asIScriptFunction *asModule_GetFunctionDescriptorById(asIScriptModule *m, int funcId)                                                   { return m->GetFunctionDescriptorById(funcId); }
	AS_API int                asModule_RemoveFunction(asIScriptModule *m, int funcId) { return m->RemoveFunction(funcId); }

	// Global variables
	AS_API int                asModule_ResetGlobalVars(asIScriptModule *m)                                                                         { return m->ResetGlobalVars(); }
	AS_API asUINT             asModule_GetGlobalVarCount(asIScriptModule *m)                                                                       { return m->GetGlobalVarCount(); }
	AS_API int                asModule_GetGlobalVarIndexByName(asIScriptModule *m, const char *name)                                               { return m->GetGlobalVarIndexByName(name); }
	AS_API int                asModule_GetGlobalVarIndexByDecl(asIScriptModule *m, const char *decl)                                               { return m->GetGlobalVarIndexByDecl(decl); }
	AS_API const char        *asModule_GetGlobalVarDeclaration(asIScriptModule *m, int index)                                                      { return m->GetGlobalVarDeclaration(index); }
	AS_API int                asModule_GetGlobalVar(asIScriptModule *m, asUINT index, const char **name, int *typeId, bool *isConst) { return m->GetGlobalVar(index, name, typeId, isConst); }
	AS_API void              *asModule_GetAddressOfGlobalVar(asIScriptModule *m, int index)                                                        { return m->GetAddressOfGlobalVar(index); }
	AS_API int                asModule_RemoveGlobalVar(asIScriptModule *m, asUINT index) { return m->RemoveGlobalVar(index); }

	// Type identification
	AS_API asUINT             asModule_GetObjectTypeCount(asIScriptModule *m)                                                                      { return m->GetObjectTypeCount(); }
	AS_API asIObjectType     *asModule_GetObjectTypeByIndex(asIScriptModule *m, asUINT index)                                                      { return m->GetObjectTypeByIndex(index); }
	AS_API int                asModule_GetTypeIdByDecl(asIScriptModule *m, const char *decl)                                                       { return m->GetTypeIdByDecl(decl); }

	// Enums
	AS_API asUINT             asModule_GetEnumCount(asIScriptModule *m)                                                                            { return m->GetEnumCount(); }
	AS_API const char *       asModule_GetEnumByIndex(asIScriptModule *m, asUINT index, int *enumTypeId)                                           { return m->GetEnumByIndex(index, enumTypeId); }
	AS_API int                asModule_GetEnumValueCount(asIScriptModule *m, int enumTypeId)                                                       { return m->GetEnumValueCount(enumTypeId); }
	AS_API const char *       asModule_GetEnumValueByIndex(asIScriptModule *m, int enumTypeId, asUINT index, int *outValue)                        { return m->GetEnumValueByIndex(enumTypeId, index, outValue); }

	// Typedefs
	AS_API asUINT             asModule_GetTypedefCount(asIScriptModule *m)                                                                         { return m->GetTypedefCount(); }
	AS_API const char *       asModule_GetTypedefByIndex(asIScriptModule *m, asUINT index, int *typeId)                                            { return m->GetTypedefByIndex(index, typeId); }

	// Dynamic binding between modules
	AS_API asUINT             asModule_GetImportedFunctionCount(asIScriptModule *m)                                                                { return m->GetImportedFunctionCount(); }
	AS_API int                asModule_GetImportedFunctionIndexByDecl(asIScriptModule *m, const char *decl)                                        { return m->GetImportedFunctionIndexByDecl(decl); }
	AS_API const char        *asModule_GetImportedFunctionDeclaration(asIScriptModule *m, int importIndex)                                         { return m->GetImportedFunctionDeclaration(importIndex); }
	AS_API const char        *asModule_GetImportedFunctionSourceModule(asIScriptModule *m, int importIndex)                                        { return m->GetImportedFunctionSourceModule(importIndex); }
	AS_API int                asModule_BindImportedFunction(asIScriptModule *m, int importIndex, int funcId)                                       { return m->BindImportedFunction(importIndex, funcId); }
	AS_API int                asModule_UnbindImportedFunction(asIScriptModule *m, int importIndex)                                                 { return m->UnbindImportedFunction(importIndex); }
	AS_API int                asModule_BindAllImportedFunctions(asIScriptModule *m)                                                                { return m->BindAllImportedFunctions(); }
	AS_API int                asModule_UnbindAllImportedFunctions(asIScriptModule *m)                                                              { return m->UnbindAllImportedFunctions(); }

	// Bytecode saving and loading
	AS_API int                asModule_SaveByteCode(asIScriptModule *m, asIBinaryStream *out)                                                      { return m->SaveByteCode(out); }
	AS_API int                asModule_LoadByteCode(asIScriptModule *m, asIBinaryStream *in)                                                       { return m->LoadByteCode(in); }

	///////////////////////////////////////////
	// asIScriptContext

	// Memory management
	AS_API int              asContext_AddRef(asIScriptContext *c)                                                               { return c->AddRef(); }
	AS_API int              asContext_Release(asIScriptContext *c)                                                              { return c->Release(); }

	// Miscellaneous
	AS_API asIScriptEngine *asContext_GetEngine(asIScriptContext *c)                                                            { return c->GetEngine(); }

	// Execution
	AS_API int              asContext_Prepare(asIScriptContext *c, int funcId)                                                  { return c->Prepare(funcId); }
	AS_API int              asContext_Unprepare(asIScriptContext *c)                                                            { return c->Unprepare(); }
	AS_API int              asContext_SetObject(asIScriptContext *c, void *obj)                                                 { return c->SetObject(obj); }
	AS_API int              asContext_Execute(asIScriptContext *c)                                                              { return c->Execute(); }
	AS_API int              asContext_Abort(asIScriptContext *c)                                                                { return c->Abort(); }
	AS_API int              asContext_Suspend(asIScriptContext *c)                                                              { return c->Suspend(); }
	AS_API int              asContext_GetState(asIScriptContext *c)                                                             { return c->GetState(); }

	// Arguments
	AS_API int              asContext_SetArgByte(asIScriptContext *c, asUINT arg, asBYTE value)                                 { return c->SetArgByte(arg, value); } 
	AS_API int              asContext_SetArgWord(asIScriptContext *c, asUINT arg, asWORD value)                                 { return c->SetArgWord(arg, value); } 
	AS_API int              asContext_SetArgDWord(asIScriptContext *c, asUINT arg, asDWORD value)                               { return c->SetArgDWord(arg, value); } 
	AS_API int              asContext_SetArgQWord(asIScriptContext *c, asUINT arg, asQWORD value)                               { return c->SetArgQWord(arg, value); }
	AS_API int              asContext_SetArgFloat(asIScriptContext *c, asUINT arg, float value)                                 { return c->SetArgFloat(arg, value); }
	AS_API int              asContext_SetArgDouble(asIScriptContext *c, asUINT arg, double value)                               { return c->SetArgDouble(arg, value); }
	AS_API int              asContext_SetArgAddress(asIScriptContext *c, asUINT arg, void *addr)                                { return c->SetArgAddress(arg, addr); }
	AS_API int              asContext_SetArgObject(asIScriptContext *c, asUINT arg, void *obj)                                  { return c->SetArgObject(arg, obj); }
	AS_API void *           asContext_GetAddressOfArg(asIScriptContext *c, asUINT arg)                                          { return c->GetAddressOfArg(arg); }

	// Return value
	AS_API asBYTE           asContext_GetReturnByte(asIScriptContext *c)                                                        { return c->GetReturnByte(); }
	AS_API asWORD           asContext_GetReturnWord(asIScriptContext *c)                                                        { return c->GetReturnWord(); }
	AS_API asDWORD          asContext_GetReturnDWord(asIScriptContext *c)                                                       { return c->GetReturnDWord(); }
	AS_API asQWORD          asContext_GetReturnQWord(asIScriptContext *c)                                                       { return c->GetReturnQWord(); }
	AS_API float            asContext_GetReturnFloat(asIScriptContext *c)                                                       { return c->GetReturnFloat(); }
	AS_API double           asContext_GetReturnDouble(asIScriptContext *c)                                                      { return c->GetReturnDouble(); }
	AS_API void *           asContext_GetReturnAddress(asIScriptContext *c)                                                     { return c->GetReturnAddress(); }
	AS_API void *           asContext_GetReturnObject(asIScriptContext *c)                                                      { return c->GetReturnObject(); }
	AS_API void *           asContext_GetAddressOfReturnValue(asIScriptContext *c)                                              { return c->GetAddressOfReturnValue(); }

	// Exception handling
	AS_API int              asContext_SetException(asIScriptContext *c, const char *string)                                     { return c->SetException(string); }
	AS_API int              asContext_GetExceptionLineNumber(asIScriptContext *c, int *column)                                  { return c->GetExceptionLineNumber(column); }
	AS_API int              asContext_GetExceptionFunction(asIScriptContext *c)                                                 { return c->GetExceptionFunction(); }
	AS_API const char *     asContext_GetExceptionString(asIScriptContext *c)                                                   { return c->GetExceptionString(); }
	AS_API int              asContext_SetExceptionCallback(asIScriptContext *c, asFUNCTION_t callback, void *obj, int callConv) { return c->SetExceptionCallback(asFUNCTION(callback), obj, callConv); }
	AS_API void             asContext_ClearExceptionCallback(asIScriptContext *c)                                               { c->ClearExceptionCallback(); }

	// Debugging
	AS_API int              asContext_SetLineCallback(asIScriptContext *c, asFUNCTION_t callback, void *obj, int callConv)      { return c->SetLineCallback(asFUNCTION(callback), obj, callConv); }
	AS_API void             asContext_ClearLineCallback(asIScriptContext *c)                                                    { c->ClearLineCallback(); }
	AS_API int              asContext_GetCallstackSize(asIScriptContext *c)                                                     { return c->GetCallstackSize(); }
	AS_API asIScriptFunction *asContext_GetFunction(asIScriptContext *c, asUINT stackLevel)                                       { return c->GetFunction(stackLevel); }
	AS_API int              asContext_GetLineNumber(asIScriptContext *c, asUINT stackLevel, int *column, const char **sectionName) { return c->GetLineNumber(stackLevel, column, sectionName); }
	AS_API int              asContext_GetVarCount(asIScriptContext *c, int stackLevel)                                          { return c->GetVarCount(stackLevel); }
	AS_API const char *     asContext_GetVarName(asIScriptContext *c, int varIndex, int stackLevel)                             { return c->GetVarName(varIndex, stackLevel); }
	AS_API const char *     asContext_GetVarDeclaration(asIScriptContext *c, int varIndex, int stackLevel)                      { return c->GetVarDeclaration(varIndex, stackLevel); }
	AS_API int              asContext_GetVarTypeId(asIScriptContext *c, int varIndex, int stackLevel)                           { return c->GetVarTypeId(varIndex, stackLevel); }
	AS_API void *           asContext_GetAddressOfVar(asIScriptContext *c, int varIndex, int stackLevel)                        { return c->GetAddressOfVar(varIndex, stackLevel); }
	AS_API bool             asContext_IsVarInScope(asIScriptContext *c, asUINT varIndex, asUINT stackLevel)                     { return c->IsVarInScope(varIndex, stackLevel); }
	AS_API int              asContext_GetThisTypeId(asIScriptContext *c, int stackLevel)                                        { return c->GetThisTypeId(stackLevel); }
	AS_API void *           asContext_GetThisPointer(asIScriptContext *c, int stackLevel)                                       { return c->GetThisPointer(stackLevel); }

	// User data
	AS_API void *           asContext_SetUserData(asIScriptContext *c, void *data)                                              { return c->SetUserData(data); }
	AS_API void *           asContext_GetUserData(asIScriptContext *c)                                                          { return c->GetUserData(); }

	///////////////////////////////////////////
	// asIScriptGeneric

	// Miscellaneous
	AS_API asIScriptEngine   *asGeneric_GetEngine(asIScriptGeneric *g)                    { return g->GetEngine(); }
	AS_API int                asGeneric_GetFunctionId(asIScriptGeneric *g)                { return g->GetFunctionId(); }
	AS_API asIScriptFunction *asGeneric_GetFunctionDescriptor(asIScriptGeneric *g) { return g->GetFunctionDescriptor(); }
	AS_API void *             asGeneric_GetFunctionUserData(asIScriptGeneric *g) { return g->GetFunctionUserData(); }

	// Object
	AS_API void *           asGeneric_GetObject(asIScriptGeneric *g)                    { return g->GetObject(); }
	AS_API int              asGeneric_GetObjectTypeId(asIScriptGeneric *g)              { return g->GetObjectTypeId(); }

	// Arguments
	AS_API int              asGeneric_GetArgCount(asIScriptGeneric *g)                  { return g->GetArgCount(); }
	AS_API int              asGeneric_GetArgTypeId(asIScriptGeneric *g, asUINT arg)     { return g->GetArgTypeId(arg); }
	AS_API asBYTE           asGeneric_GetArgByte(asIScriptGeneric *g, asUINT arg)       { return g->GetArgByte(arg); }
	AS_API asWORD           asGeneric_GetArgWord(asIScriptGeneric *g, asUINT arg)       { return g->GetArgWord(arg); }
	AS_API asDWORD          asGeneric_GetArgDWord(asIScriptGeneric *g, asUINT arg)      { return g->GetArgDWord(arg); }
	AS_API asQWORD          asGeneric_GetArgQWord(asIScriptGeneric *g, asUINT arg)      { return g->GetArgQWord(arg); }
	AS_API float            asGeneric_GetArgFloat(asIScriptGeneric *g, asUINT arg)      { return g->GetArgFloat(arg); }
	AS_API double           asGeneric_GetArgDouble(asIScriptGeneric *g, asUINT arg)     { return g->GetArgDouble(arg); }
	AS_API void *           asGeneric_GetArgAddress(asIScriptGeneric *g, asUINT arg)    { return g->GetArgAddress(arg); }
	AS_API void *           asGeneric_GetArgObject(asIScriptGeneric *g, asUINT arg)     { return g->GetArgObject(arg); }
	AS_API void *           asGeneric_GetAddressOfArg(asIScriptGeneric *g, asUINT arg)  { return g->GetAddressOfArg(arg); }

	// Return value
	AS_API int              asGeneric_GetReturnTypeId(asIScriptGeneric *g)              { return g->GetReturnTypeId(); }
	AS_API int              asGeneric_SetReturnByte(asIScriptGeneric *g, asBYTE val)    { return g->SetReturnByte(val); }
	AS_API int              asGeneric_SetReturnWord(asIScriptGeneric *g, asWORD val)    { return g->SetReturnWord(val); }
	AS_API int              asGeneric_SetReturnDWord(asIScriptGeneric *g, asDWORD val)  { return g->SetReturnDWord(val); }
	AS_API int              asGeneric_SetReturnQWord(asIScriptGeneric *g, asQWORD val)  { return g->SetReturnQWord(val); }
	AS_API int              asGeneric_SetReturnFloat(asIScriptGeneric *g, float val)    { return g->SetReturnFloat(val); }
	AS_API int              asGeneric_SetReturnDouble(asIScriptGeneric *g, double val)  { return g->SetReturnDouble(val); }
	AS_API int              asGeneric_SetReturnAddress(asIScriptGeneric *g, void *addr) { return g->SetReturnAddress(addr); }
	AS_API int              asGeneric_SetReturnObject(asIScriptGeneric *g, void *obj)   { return g->SetReturnObject(obj); }
	AS_API void *           asGeneric_GetAddressOfReturnLocation(asIScriptGeneric *g)   { return g->GetAddressOfReturnLocation(); }

	///////////////////////////////////////////
	// asIScriptObject

	// Memory management
	AS_API int              asObject_AddRef(asIScriptObject *s)                           { return s->AddRef(); }
	AS_API int              asObject_Release(asIScriptObject *s)                          { return s->Release(); }

	// Type info
	AS_API int              asObject_GetTypeId(asIScriptObject *s)                        { return s->GetTypeId(); }
	AS_API asIObjectType *  asObject_GetObjectType(asIScriptObject *s)                    { return s->GetObjectType(); }

	// Class properties
	AS_API int              asObject_GetPropertyCount(asIScriptObject *s)                 { return s->GetPropertyCount(); }
	AS_API int              asObject_GetPropertyTypeId(asIScriptObject *s, asUINT prop)   { return s->GetPropertyTypeId(prop); }
	AS_API const char *     asObject_GetPropertyName(asIScriptObject *s, asUINT prop)     { return s->GetPropertyName(prop); }
	AS_API void *           asObject_GetAddressOfProperty(asIScriptObject *s, asUINT prop)  { return s->GetAddressOfProperty(prop); }

	AS_API asIScriptEngine *asObject_GetEngine(asIScriptObject *s)                        { return s->GetEngine(); }
	AS_API int              asObject_CopyFrom(asIScriptObject *s, asIScriptObject *other) { return s->CopyFrom(other); }

	///////////////////////////////////////////
	// asIObjectType

	AS_API asIScriptEngine *asObjectType_GetEngine(const asIObjectType *o)                                                      { return o->GetEngine(); }
	AS_API const char      *asObjectType_GetConfigGroup(const asIObjectType *o) { return o->GetConfigGroup(); }

	// Memory management
	AS_API int asObjectType_AddRef(asIObjectType *o)                                                         { return o->AddRef(); }
	AS_API int asObjectType_Release(asIObjectType *o)                                                        { return o->Release(); }

	// Type info
	AS_API const char      *asObjectType_GetName(const asIObjectType *o)                                                        { return o->GetName(); }
	AS_API asIObjectType   *asObjectType_GetBaseType(const asIObjectType *o)                                                    { return o->GetBaseType(); }
	AS_API asDWORD          asObjectType_GetFlags(const asIObjectType *o)                                                       { return o->GetFlags(); }
	AS_API asUINT           asObjectType_GetSize(const asIObjectType *o)                                                        { return o->GetSize(); }
	AS_API int              asObjectType_GetTypeId(const asIObjectType *o)                                                      { return o->GetTypeId(); }
	AS_API int              asObjectType_GetSubTypeId(const asIObjectType *o)                                                   { return o->GetSubTypeId(); }

	// Interfaces
	AS_API asUINT           asObjectType_GetInterfaceCount(const asIObjectType *o)                                              { return o->GetInterfaceCount(); }
	AS_API asIObjectType   *asObjectType_GetInterface(const asIObjectType *o, asUINT index)                                     { return o->GetInterface(index); }

	// Factories
	AS_API asUINT             asObjectType_GetFactoryCount(const asIObjectType *o)                                                { return o->GetFactoryCount(); }
	AS_API int                asObjectType_GetFactoryIdByIndex(const asIObjectType *o, int index)                                 { return o->GetFactoryIdByIndex(index); }
	AS_API int                asObjectType_GetFactoryIdByDecl(const asIObjectType *o, const char *decl)                           { return o->GetFactoryIdByDecl(decl); }

	// Methods
	AS_API asUINT             asObjectType_GetMethodCount(const asIObjectType *o)                                                 { return o->GetMethodCount(); }
	AS_API int                asObjectType_GetMethodIdByIndex(const asIObjectType *o, int index)                                  { return o->GetMethodIdByIndex(index); }
	AS_API int                asObjectType_GetMethodIdByName(const asIObjectType *o, const char *name)                            { return o->GetMethodIdByName(name); }
	AS_API int                asObjectType_GetMethodIdByDecl(const asIObjectType *o, const char *decl)                            { return o->GetMethodIdByDecl(decl); }
	AS_API asIScriptFunction *asObjectType_GetMethodDescriptorByIndex(const asIObjectType *o, int index)                          { return o->GetMethodDescriptorByIndex(index); }

	// Properties
	AS_API asUINT      asObjectType_GetPropertyCount(const asIObjectType *o)                                               { return o->GetPropertyCount(); }
	AS_API int         asObjectType_GetProperty(const asIObjectType *o, asUINT index, const char **name, int *typeId, bool *isPrivate, int *offset, bool *isReference) { return o->GetProperty(index, name, typeId, isPrivate, offset, isReference); }
	AS_API const char *asObjectType_GetPropertyDeclaration(const asIObjectType *o, asUINT index) { return o->GetPropertyDeclaration(index); }

	// Behaviours
	AS_API asUINT asObjectType_GetBehaviourCount(const asIObjectType *o)                                              { return o->GetBehaviourCount(); }
	AS_API int    asObjectType_GetBehaviourByIndex(const asIObjectType *o, asUINT index, asEBehaviours *outBehaviour) { return o->GetBehaviourByIndex(index, outBehaviour); }

	///////////////////////////////////////////
	// asIScriptFunction

	AS_API asIScriptEngine *asFunction_GetEngine(const asIScriptFunction *f)                                           { return f->GetEngine(); }

	// Memory management
	AS_API int              asFunction_AddRef(const asIScriptFunction *f) { return f->AddRef(); }
	AS_API int              asFunction_Release(const asIScriptFunction *f) { return f->Release(); }

	AS_API int              asFunction_GetId(const asIScriptFunction *f) { return f->GetId(); }
	AS_API asEFuncType      asFunction_GetFuncType(const asIScriptFunction *f) { return f->GetFuncType(); }
	AS_API const char      *asFunction_GetModuleName(const asIScriptFunction *f)                                       { return f->GetModuleName(); }
	AS_API const char      *asFunction_GetScriptSectionName(const asIScriptFunction *f) { return f->GetScriptSectionName(); }
	AS_API const char      *asFunction_GetConfigGroup(const asIScriptFunction *f) { return f->GetConfigGroup(); }
	AS_API asIObjectType   *asFunction_GetObjectType(const asIScriptFunction *f)                                       { return f->GetObjectType(); }
	AS_API const char      *asFunction_GetObjectName(const asIScriptFunction *f)                                       { return f->GetObjectName(); }
	AS_API const char      *asFunction_GetName(const asIScriptFunction *f)                                             { return f->GetName(); }
	AS_API const char      *asFunction_GetDeclaration(const asIScriptFunction *f, bool includeObjectName)              { return f->GetDeclaration(includeObjectName); }
	AS_API bool             asFunction_IsReadOnly(const asIScriptFunction *f) { return f->IsReadOnly(); }
	AS_API bool             asFunction_IsPrivate(const asIScriptFunction *f) { return f->IsPrivate(); }

	AS_API asUINT           asFunction_GetParamCount(const asIScriptFunction *f)                                       { return f->GetParamCount(); }
	AS_API int              asFunction_GetParamTypeId(const asIScriptFunction *f, int index)                           { return f->GetParamTypeId(index); }
	AS_API int              asFunction_GetReturnTypeId(const asIScriptFunction *f)                                     { return f->GetReturnTypeId(); }

	// Debug information
	AS_API asUINT           asFunction_GetVarCount(const asIScriptFunction *f) { return f->GetVarCount(); }
	AS_API int              asFunction_GetVar(const asIScriptFunction *f, asUINT index, const char **name, int *typeId) { return f->GetVar(index, name, typeId); }
	AS_API const char *     asFunction_GetVarDecl(const asIScriptFunction *f, asUINT index) { return f->GetVarDecl(index); }
	AS_API int              asFunction_FindNextLineWithCode(const asIScriptFunction *f, int line) { return f->FindNextLineWithCode(line); }

	// For JIT compilation
	AS_API asDWORD         *asFunction_GetByteCode(asIScriptFunction *f, asUINT *length) { return f->GetByteCode(length); }

	// User data
	AS_API void            *asFunction_SetUserData(asIScriptFunction *f, void *userData) { return f->SetUserData(userData); }
	AS_API void            *asFunction_GetUserData(const asIScriptFunction *f) { return f->GetUserData(); }

}

END_AS_NAMESPACE



