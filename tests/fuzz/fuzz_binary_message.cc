/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "custom_memory_manager.h"
#include "ua_server_internal.h"
#include "ua_config_default.h"
#include "ua_log_stdout.h"
#include "ua_plugin_log.h"
#include "testing_networklayers.h"

#define RECEIVE_BUFFER_SIZE 65535

/*
** Main entry point.  The fuzzer invokes this function with each
** fuzzed input.
*/
extern "C" int
LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {

    if (!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;
    size -= 4;

    UA_Connection c = createDummyConnection(RECEIVE_BUFFER_SIZE, NULL);
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    if (!config) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create server config using UA_ServerConfig_new_default");
        return 0;
    }
    UA_Server *server = UA_Server_new(config);
    if (server == NULL) {
        UA_ServerConfig_delete(config);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not create server instance using UA_Server_new");
        return 0;
    }

    // we need to copy the message because it will be freed in the processing function
    UA_ByteString msg = UA_ByteString();
    UA_StatusCode retval = UA_ByteString_allocBuffer(&msg, size);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_ServerConfig_delete(config);
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                     "Could not allocate message buffer");
        return 0;
    }
    memcpy(msg.data, data, size);

    UA_Server_processBinaryMessage(server, &c, &msg);
    // if we got an invalid chunk, the message is not deleted, so delete it here
    UA_ByteString_deleteMembers(&msg);
    UA_Server_run_shutdown(server);
    UA_Server_delete(server);
    UA_ServerConfig_delete(config);
    c.close(&c);
    UA_Connection_deleteMembers(&c);
    return 0;
}