LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= factory-test-client.c data.c
LOCAL_MODULE:= factory-test-client
LOCAL_32_BIT_ONLY := true

include $(BUILD_EXECUTABLE)
