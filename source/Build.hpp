///                                                                           
/// Langulus::Profiler                                                        
/// Copyright (c) 2025 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: MIT                                              
///                                                                           
#pragma once
#include <Langulus/Core/Config.hpp>
#include <bitset>

#if not LANGULUS_FEATURE(PROFILING)
   #error This file shouldn't be included if LANGULUS_FEATURE_PROFILING is disabled
#endif

namespace Langulus::Profiler
{
   
   ///                                                                        
   /// A build configuration                                                  
   ///                                                                        
#pragma pack(push, 1)
   struct Build {
      enum Property {
         Safe = 0,
         Test,
         Benchmark,
         Paranoia,
         Debug,

         ManagedReflection,
         ManagedMemory,
         MemoryStatistics,
         OverrideNewDelete,
         Unicode,
         Compression,
         Encryption,

         CompilerGCC,
         CompilerMSVC,
         CompilerClang,
         CompilerWASM,
         CompilerMinGW,

         OSWindows,
         OSLinux,
         OSAndroid,
         OSMacos,
         OSUnix,
         OSFreeBSD,

         LoggerFatalError,
         LoggerError,
         LoggerWarning,
         LoggerVerbose,
         LoggerInfo,
         LoggerMessage,
         LoggerSpecial,
         LoggerFlow,
         LoggerInput,
         LoggerNetwork,
         LoggerOS,
         LoggerPrompt,

         SIMD,
         AVX512BW,
         AVX512CD,
         AVX512DQ,
         AVX512F,
         AVX512VL,
         AVX512,
         AVX2,
         AVX,
         SSE4_2,
         SSE4_1,
         SSSE3,
         SSE3,
         SSE2,
         SSE,

         Counter
      };

      ::std::bitset<Property::Counter> properties = 0;
      uint8_t bitness = 0;
      uint8_t alignment = 0;
      uint8_t endianness = 0;

      constexpr Build() noexcept;

      constexpr bool operator == (const Build& rhs) const noexcept {
         return properties == rhs.properties
            and bitness == rhs.bitness
            and alignment == rhs.alignment
            and endianness == rhs.endianness;
      }
   };
#pragma pack(pop)

} // namespace Langulus::Profiler

namespace std
{

   template<>
   struct hash<::Langulus::Profiler::Build> {
      using B = ::Langulus::Profiler::Build;

      size_t operator()(const B& what) const noexcept {
         size_t ph = hash<decltype(B::properties)> {}(what.properties);
         return ph ^ ((what.bitness << 24) | (what.alignment << 16) | (what.endianness << 8));
      }
   };

} // namespace std

namespace Langulus::Profiler
{
   
   /// Generate a build ID                                                    
   /// Should always be inlined and invoked from the place we're profiling    
   /// for most accurate results                                              
   LANGULUS(ALWAYS_INLINED)
   constexpr Build::Build() noexcept {
      properties[Safe]              = LANGULUS_SAFE();
      properties[Test]              = LANGULUS_TESTING();
      properties[Benchmark]         = LANGULUS_BENCHMARK();
      properties[Paranoia]          = LANGULUS_PARANOID();
      properties[Debug]             = LANGULUS_DEBUG();

      properties[ManagedReflection] = LANGULUS_FEATURE_MANAGED_REFLECTION();
      properties[ManagedMemory]     = LANGULUS_FEATURE_MANAGED_MEMORY();
      properties[MemoryStatistics]  = LANGULUS_FEATURE_MEMORY_STATISTICS();
      properties[OverrideNewDelete] = LANGULUS_FEATURE_NEWDELETE();
      properties[Unicode]           = LANGULUS_FEATURE_UNICODE();
      properties[Compression]       = LANGULUS_FEATURE_COMPRESSION();
      properties[Encryption]        = LANGULUS_FEATURE_ENCRYPTION();

      properties[CompilerGCC]       = LANGULUS_COMPILER_GCC();
      properties[CompilerMSVC]      = LANGULUS_COMPILER_MSVC();
      properties[CompilerClang]     = LANGULUS_COMPILER_CLANG();
      properties[CompilerWASM]      = LANGULUS_COMPILER_WASM();
      properties[CompilerMinGW]     = LANGULUS_COMPILER_MINGW();

      properties[OSWindows]         = LANGULUS_OS_WINDOWS();
      properties[OSLinux]           = LANGULUS_OS_LINUX();
      properties[OSAndroid]         = LANGULUS_OS_ANDROID();
      properties[OSMacos]           = LANGULUS_OS_MACOS();
      properties[OSUnix]            = LANGULUS_OS_UNIX();
      properties[OSFreeBSD]         = LANGULUS_OS_FREEBSD();

      #ifdef LANGULUS_LOGGER_ENABLE_FATALERRORS
         properties[LoggerFatalError] = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_ERRORS
         properties[LoggerError]    = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_WARNINGS
         properties[LoggerWarning]  = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_VERBOSE
         properties[LoggerVerbose]  = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_INFOS
         properties[LoggerInfo]     = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_MESSAGES
         properties[LoggerMessage]  = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_SPECIALS
         properties[LoggerSpecial]  = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_FLOWS
         properties[LoggerFlow]     = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_INPUTS
         properties[LoggerInput]    = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_INPUTS
         properties[LoggerNetwork]  = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_OS
         properties[LoggerOS]       = true;
      #endif
      #ifdef LANGULUS_LOGGER_ENABLE_PROMPTS
         properties[LoggerPrompt]   = true;
      #endif

      #ifdef LANGULUS_LIBRARY_SIMD
         properties[SIMD]           = LANGULUS_SIMD_ENABLED();
         properties[AVX512BW]       = LANGULUS_SIMD_AVX512BW();
         properties[AVX512CD]       = LANGULUS_SIMD_AVX512CD();
         properties[AVX512DQ]       = LANGULUS_SIMD_AVX512DQ();
         properties[AVX512F]        = LANGULUS_SIMD_AVX512F();
         properties[AVX512VL]       = LANGULUS_SIMD_AVX512VL();
         properties[AVX512]         = LANGULUS_SIMD_AVX512();
         properties[AVX2]           = LANGULUS_SIMD_AVX2();
         properties[AVX]            = LANGULUS_SIMD_AVX();
         properties[SSE4_2]         = LANGULUS_SIMD_SSE4_2();
         properties[SSE4_1]         = LANGULUS_SIMD_SSE4_1();
         properties[SSSE3]          = LANGULUS_SIMD_SSSE3();
         properties[SSE3]           = LANGULUS_SIMD_SSE3();
         properties[SSE2]           = LANGULUS_SIMD_SSE2();
         properties[SSE]            = LANGULUS_SIMD_SSE();
      #endif

      //                                                                
      bitness = Bitness;
      alignment = Alignment;

      //                                                                
      if constexpr (BigEndianMachine != LittleEndianMachine)
         endianness = BigEndianMachine ? 1 : 2;
      else
         endianness = 0;
   }

} // namespace Langulus::Profiler