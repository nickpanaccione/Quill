#include "CompileService.h"
#include <juce_events/juce_events.h>

CompileService::CompileService(juce::File compilerPath, juce::File sharedDir)
  : juce::Thread("compile"),
    compiler(std::move(compilerPath)),
    sharedIncludeDir(std::move(sharedDir)) {
  startThread();
}

CompileService::~CompileService() {
  stopThread(5000);
}

void CompileService::compile(juce::File sourceFile, juce::File outputDir, Callback callback) {
  {
    const juce::ScopedLock sl(lock);
    pending = std::make_unique<Request>(Request {
      std::move(sourceFile), std::move(outputDir), std::move(callback)
    });
  }
  notify();
}

void CompileService::run() {
  while (!threadShouldExit()) {
    wait(100);

    std::unique_ptr<Request> req;
    {
      const juce::ScopedLock sl(lock);
      req = std::move(pending);
    }

    if (req == nullptr) {
      continue;
    }

    auto outputLib = req->outputDir.getChildFile("dsp.dylib");

    // clang++ find libc++ 
    juce::String cmd;
    cmd << "'" << compiler.getFullPathName() << "'"
        << " -dynamiclib -std=c++20 -O1"
        << " -isysroot $(xcrun --show-sdk-path)"
        << " -I'" << sharedIncludeDir.getFullPathName() << "'"
        << " -o '" << outputLib.getFullPathName() << "'"
        << " '" << req->sourceFile.getFullPathName() << "'"
        << " 2>&1";

    juce::ChildProcess proc;
    Result result;

    if (!proc.start(juce::StringArray { "/bin/sh", "-c", cmd })) {
      result.compilerOutput = "failed to launch compiler process";
    } else {
      result.compilerOutput = proc.readAllProcessOutput();
      proc.waitForProcessToFinish(30000);
      result.success = (proc.getExitCode() == 0);
      if (result.success) {
        result.outputLib = outputLib;
      }
    }

    Callback cb = std::move(req->callback);
    juce::MessageManager::callAsync([fn = std::move(cb), r = std::move(result)]() mutable {
      fn(std::move(r));
    });
  }
}
