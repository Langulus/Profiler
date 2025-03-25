///                                                                           
/// Langulus::Profiler                                                        
/// Copyright (c) 2025 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: MIT                                              
///                                                                           
#include <Langulus/Logger.hpp>

#if LANGULUS_FEATURE(PROFILING)
#include <chrono>
#include <string>
#include <vector>
#include <memory>


#if defined(LANGULUS_EXPORT_ALL) or defined(LANGULUS_EXPORT_PROFILER)
   #define LANGULUS_API_PROFILER() LANGULUS_EXPORT()
#else
   #define LANGULUS_API_PROFILER() LANGULUS_IMPORT()
#endif

/// Make the rest of the code aware, that Langulus::Profiler has been included
#define LANGULUS_LIBRARY_PROFILER() 1


namespace Langulus::Profiler
{

   using Clock = ::std::chrono::steady_clock;
   using TimePoint = Clock::time_point;
   using Time = Clock::duration;
   using String = ::std::string;
   using Nano = ::std::chrono::duration<long double, ::std::nano>;
   using namespace ::std::chrono_literals;

   LANGULUS(ALWAYS_INLINED)
   long double RealMs(Time t) noexcept {
      return ::std::chrono::duration_cast<Nano>(t).count() / 1'000'000.0;
   }

   ///                                                                        
   /// The profiler state object, keeping track of running measurements       
   ///                                                                        
   struct State {
      struct Measurement;
      struct Result;
      struct Stopper;

      using ResultPtr = ::std::unique_ptr<Result>;
      using MeasurementPtr = ::std::shared_ptr<Measurement>;

   private:
      ::std::shared_ptr<Measurement> main;
      ::std::unordered_map<String, ResultPtr> results;
      String output_file = "profiling.md";
      Time output_interval = 1s;
      TimePoint last_output_timestamp = Clock::now();

      LANGULUS_API(PROFILER) void CompileAndDump();
      LANGULUS_API(PROFILER) void DumpProfilerResults() const;
      LANGULUS_API(PROFILER) void DumpInner(::std::ofstream&, int, const Result&, int, Time) const;

   public:
      LANGULUS_API(PROFILER) void Configure(String&&, Time) noexcept;
      LANGULUS_API(PROFILER) ~State();

      LANGULUS_API(PROFILER) auto Start(String&&) -> Stopper;
   };


   /// Global profiler instance                                               
   LANGULUS_API(PROFILER) extern State Instance;


   ///                                                                        
   /// A single measurement                                                   
   ///                                                                        
   struct State::Measurement {
   protected:
      friend struct State;
      String    name;
      TimePoint start;
      TimePoint end;
      bool      running = true;
      ::std::vector<MeasurementPtr> children;

   public:
      Measurement() = delete;

      LANGULUS_API(PROFILER) Measurement(String&&) noexcept;
      LANGULUS_API(PROFILER) void Stop() noexcept;
   };


   ///                                                                        
   /// Auto measurement stopper on scope end                                  
   ///                                                                        
   struct State::Stopper {
   private:
      MeasurementPtr measurement;

   public:
      Stopper() = default;
      Stopper(const Stopper&) = delete;

      LANGULUS_API(PROFILER)  Stopper(const MeasurementPtr&) noexcept;
      LANGULUS_API(PROFILER) ~Stopper();
   };


   ///                                                                        
   /// A compiled result                                                      
   ///                                                                        
   struct State::Result {
      const Result* parent = nullptr;
      String name;
      Time min, max, average;
      long long samples;
      ::std::unordered_map<String, ResultPtr> children;

      LANGULUS_API(PROFILER) void Integrate(bool isNew, const Measurement&);
   };


   /// Start doing a measurement                                              
   ///   @param n - name of the measurement, usually the function name        
   ///   @return the auto-stopper                                             
   LANGULUS(ALWAYS_INLINED)
   State::Stopper Start(String&& n) {
      return Instance.Start(::std::forward<String>(n));
   }

} // namespace Langulus::Profiler

#undef LANGULUS_PROFILE

/// Start a scoped profiling                                                  
#define LANGULUS_PROFILE() \
   const auto scoped_profiler____________ = ::Langulus::Profiler::Start(LANGULUS(FUNCTION))

#endif