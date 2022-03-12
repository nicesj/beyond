/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <beyond/private/beyond_private.h>

#define MODULE_FILENAME "libbeyond-" BEYOND_PLUGIN_PEER_NN_NAME ".so"
#define MODEL_FILENAME (GetModelFilename())
#define MODEL_POSENET_FILENAME (GetModelFilename(true))
#define PREPROCESSING "video/x-raw,format=RGB,width=224,height=224,framerate=0/1 ! videoconvert ! videoscale ! video/x-raw,format=RGB,width=224,height=224"
#define FILENAME_MAXSIZE 1024

class PeerTest : public testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;

protected:
    void *handle;
    beyond::ModuleInterface::EntryPoint entry;
};

struct GrpcContext {
    void *handle;
    beyond::InferenceInterface::PeerInterface *peer;
};

extern void StopGrpcServer(void);

extern void StartGrpcServer(beyond::AuthenticatorInterface *&auth, beyond::AuthenticatorInterface *&authCA);
extern void StartGrpcServer(beyond::AuthenticatorInterface *&auth);
extern void StartGrpcServer(beyond_peer_info *info);
extern void StartGrpcServer(void);

extern const char *GetModelFilename(bool poseNet = false);
