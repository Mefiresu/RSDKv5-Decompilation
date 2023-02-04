#ifndef COMMONLEGACY_HPP
#define COMMONLEGACY_HPP

/**
 * This extra header helps reduce global/static variables used in both v3/v4 and
 * even some from v5 as well.
 * It works by grouping common variables into unions/creating aliases in
 * the correct namespace so that the rest of the code can use them as normal
 * without requiring a code refactor.
 * In C++, an attribute alias needs the mangled name when using namespaces.
 * Since this is a nightmare to handle, new variables that fuse v3/v4 are
 * defined in the global namespace.
 *
 * NOTE: This only works because v3/v4/v5 will never run at the same time.
 *
 * TODO: This can definitely be refactored with some fancy macros
 */

// Declare a global variable that aliases the `aliasedvar` given in parameter
#define MAKE_ALIAS(aliasedvar,type,globalvar) \
    extern __attribute__((alias(# aliasedvar))) decltype(type) globalvar


union ObjectScriptList {
    RSDK::Legacy::v3::ObjectScript v3objectScriptList[LEGACY_v3_OBJECT_COUNT];
    RSDK::Legacy::v4::ObjectScript v4objectScriptList[LEGACY_v4_OBJECT_COUNT];
} _objectScriptList;

union ScriptFunctionList {
    RSDK::Legacy::v3::ScriptFunction v3scriptFunctionList[LEGACY_v3_FUNCTION_COUNT];
    RSDK::Legacy::v4::ScriptFunction v4scriptFunctionList[LEGACY_v4_FUNCTION_COUNT];
} _scriptFunctionList;

union TypeNames {
    char v3typeNames[LEGACY_v3_OBJECT_COUNT][0x40];
    char v4typeNames[LEGACY_v4_OBJECT_COUNT][0x40];
} _typeNames;

union ScriptCode {
    int32 v3scriptCode[LEGACY_v3_SCRIPTDATA_COUNT];
    int32 v4scriptCode[LEGACY_v4_SCRIPTCODE_COUNT];
} _scriptCode;

union JumpTable {
    int32 v3jumpTable[LEGACY_v3_JUMPTABLE_COUNT];
    int32 v4jumpTable[LEGACY_v4_JUMPTABLE_COUNT];
} _jumpTable;

union JumpTableStack {
    int32 v3jumpTableStack[LEGACY_v3_JUMPSTACK_COUNT];
    int32 v4jumpTableStack[LEGACY_v4_JUMPSTACK_COUNT];
} _jumpTableStack;

union FunctionStack {
    int32 v3functionStack[LEGACY_v3_FUNCSTACK_COUNT];
    int32 v4functionStack[LEGACY_v4_FUNCSTACK_COUNT];
} _functionStack;

union FaceBuffer {
    RSDK::Legacy::Face v3faceBuffer[LEGACY_v3_FACEBUFFER_SIZE];
    RSDK::Legacy::Face v4faceBuffer[LEGACY_v4_FACEBUFFER_SIZE];
} _faceBuffer;

union VertexBuffer {
    RSDK::Legacy::Vertex v3vertexBuffer[LEGACY_v3_VERTEXBUFFER_SIZE];
    RSDK::Legacy::Vertex v4vertexBuffer[LEGACY_v4_VERTEXBUFFER_SIZE];
} _vertexBuffer;

union VertexBufferT {
    RSDK::Legacy::Vertex v3vertexBufferT[LEGACY_v3_VERTEXBUFFER_SIZE];
    RSDK::Legacy::Vertex v4vertexBufferT[LEGACY_v4_VERTEXBUFFER_SIZE];
} _vertexBufferT;

union DrawList3D {
    RSDK::Legacy::DrawListEntry3D v3drawList3D[LEGACY_v3_FACEBUFFER_SIZE];
    RSDK::Legacy::DrawListEntry3D v4drawList3D[LEGACY_v4_FACEBUFFER_SIZE];
} _drawList3D;

namespace RSDK {
namespace Legacy {

namespace v3 {
    MAKE_ALIAS(_objectScriptList, ObjectScriptList::v3objectScriptList, objectScriptList);
    MAKE_ALIAS(_scriptFunctionList, ScriptFunctionList::v3scriptFunctionList, scriptFunctionList);

    MAKE_ALIAS(_typeNames, TypeNames::v3typeNames, typeNames);

    MAKE_ALIAS(_scriptCode, ScriptCode::v3scriptCode, scriptCode);
    MAKE_ALIAS(_jumpTable, JumpTable::v3jumpTable, jumpTable);
    MAKE_ALIAS(_jumpTableStack, JumpTableStack::v3jumpTableStack, jumpTableStack);
    MAKE_ALIAS(_functionStack, FunctionStack::v3functionStack, functionStack);

    MAKE_ALIAS(_faceBuffer, FaceBuffer::v3faceBuffer, faceBuffer);
    MAKE_ALIAS(_vertexBuffer, VertexBuffer::v3vertexBuffer, vertexBuffer);
    MAKE_ALIAS(_vertexBufferT, VertexBufferT::v3vertexBufferT, vertexBufferT);
    MAKE_ALIAS(_drawList3D, DrawList3D::v3drawList3D, drawList3D);

    // v5 common
    extern __attribute__((alias("_ZN4RSDK16objectEntityListE"))) Entity objectEntityList[LEGACY_v3_ENTITY_COUNT];
}

namespace v4 {
    MAKE_ALIAS(_objectScriptList, ObjectScriptList::v4objectScriptList, objectScriptList);
    MAKE_ALIAS(_scriptFunctionList, ScriptFunctionList::v4scriptFunctionList, scriptFunctionList);

    MAKE_ALIAS(_typeNames, TypeNames::v4typeNames, typeNames);

    MAKE_ALIAS(_scriptCode, ScriptCode::v4scriptCode, scriptCode);
    MAKE_ALIAS(_jumpTable, JumpTable::v4jumpTable, jumpTable);
    MAKE_ALIAS(_jumpTableStack, JumpTableStack::v4jumpTableStack, jumpTableStack);
    MAKE_ALIAS(_functionStack, FunctionStack::v4functionStack, functionStack);

    MAKE_ALIAS(_faceBuffer, FaceBuffer::v4faceBuffer, faceBuffer);
    MAKE_ALIAS(_vertexBuffer, VertexBuffer::v4vertexBuffer, vertexBuffer);
    MAKE_ALIAS(_vertexBufferT, VertexBufferT::v4vertexBufferT, vertexBufferT);
    MAKE_ALIAS(_drawList3D, DrawList3D::v4drawList3D, drawList3D);

    // v5 common
    extern __attribute__((alias("_ZN4RSDK16objectEntityListE"))) Entity objectEntityList[LEGACY_v4_ENTITY_COUNT * 2];
}

} // namespace Legacy
} // namespace RSDK

#endif /* COMMONLEGACY_HPP */
