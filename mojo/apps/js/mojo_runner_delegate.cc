// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "mojo/apps/js/mojo_runner_delegate.h"

#include "base/bind.h"
#include "base/path_service.h"
#include "gin/converter.h"
#include "gin/modules/console.h"
#include "gin/modules/module_registry.h"
#include "gin/modules/timer.h"
#include "gin/try_catch.h"
#include "mojo/apps/js/bindings/core.h"
#include "mojo/apps/js/bindings/gl/module.h"
#include "mojo/apps/js/bindings/monotonic_clock.h"
#include "mojo/apps/js/bindings/support.h"
#include "mojo/apps/js/bindings/threading.h"

namespace mojo {
namespace apps {

namespace {

// TODO(abarth): Rather than loading these modules from the file system, we
// should load them from the network via Mojo IPC.
std::vector<base::FilePath> GetModuleSearchPaths() {
  std::vector<base::FilePath> search_paths(2);
  PathService::Get(base::DIR_SOURCE_ROOT, &search_paths[0]);
  PathService::Get(base::DIR_EXE, &search_paths[1]);
  search_paths[1] = search_paths[1].AppendASCII("gen");
  return search_paths;
}

void StartCallback(base::WeakPtr<gin::Runner> runner,
                   MojoHandle pipe,
                   v8::Handle<v8::Value> module) {
  v8::Isolate* isolate = runner->isolate();
  v8::Handle<v8::Function> start;
  CHECK(gin::ConvertFromV8(isolate, module, &start));

  v8::Handle<v8::Value> args[] = { gin::ConvertToV8(isolate, pipe) };
  runner->Call(start, runner->global(), 1, args);
}

}  // namespace

MojoRunnerDelegate::MojoRunnerDelegate()
    : ModuleRunnerDelegate(GetModuleSearchPaths()) {
  AddBuiltinModule(gin::Console::kModuleName, gin::Console::GetModule);
  AddBuiltinModule(gin::TimerModule::kName, gin::TimerModule::GetModule);
  AddBuiltinModule(js::Core::kModuleName, js::Core::GetModule);
  AddBuiltinModule(js::Support::kModuleName, js::Support::GetModule);
  AddBuiltinModule(mojo::js::gl::kModuleName, mojo::js::gl::GetModule);
  AddBuiltinModule(MonotonicClock::kModuleName, MonotonicClock::GetModule);
  AddBuiltinModule(Threading::kModuleName, Threading::GetModule);
}

MojoRunnerDelegate::~MojoRunnerDelegate() {
}

void MojoRunnerDelegate::Start(gin::Runner* runner,
                               MojoHandle pipe,
                               const std::string& module) {
  gin::Runner::Scope scope(runner);
  gin::ModuleRegistry* registry = gin::ModuleRegistry::From(runner->context());
  registry->LoadModule(runner->isolate(), module,
                       base::Bind(StartCallback, runner->GetWeakPtr(), pipe));
  AttemptToLoadMoreModules(runner);
}

void MojoRunnerDelegate::UnhandledException(gin::Runner* runner,
                                            gin::TryCatch& try_catch) {
  gin::ModuleRunnerDelegate::UnhandledException(runner, try_catch);
  LOG(ERROR) << try_catch.GetStackTrace();
}

}  // namespace apps
}  // namespace mojo
