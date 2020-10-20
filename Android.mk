LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= factory_test_client.c data.c factory_test_function.c serial_test.c can_test.c
LOCAL_MODULE:= factory_test_client
LOCAL_32_BIT_ONLY := true

include $(BUILD_EXECUTABLE)
