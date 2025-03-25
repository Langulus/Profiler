///                                                                           
/// Langulus::Profiler                                                        
/// Copyright (c) 2025 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: MIT                                              
///                                                                           
#include <Langulus/Profiler.hpp>

#if not LANGULUS_FEATURE(PROFILING)
   #error This file shouldn't be built at all if LANGULUS_FEATURE_PROFILING is disabled
#endif


namespace Langulus::Profiler
{

   State Instance {};


   /// Configure the profiler                                                 
   ///   @param profiling_file - file to write results into                   
   ///   @param update_interval - interval at which to write to file          
   ///      higher frequency might affect performance measurements            
   void State::Configure(String&& profiling_file, Time update_interval) noexcept {
      output_file = ::std::forward<String>(profiling_file);
      output_interval = update_interval;
      last_output_timestamp = Clock::now();
   }

   State::~State() {
      DumpProfilerResults();
   }

   /// Begin a scoped measurement                                             
   ///   @param n - the mame of the measurement, usually the function name    
   ///   @return the auto-stopper                                             
   auto State::Start(String&& n) -> Stopper {
      if (main) {
         auto found = main.get();
         while (found and found->running) {
            // A measurement is already running, so find the last one   
            // in the hierarchy                                         
            if (not found->children.empty())
               found = found->children.rbegin()->get();
            else
               break;
         }

         if (found) {
            if (not found->running and found == main.get()) {
               // Main measurement has ended, it is time to compile     
               // the results and reset the state                       
               CompileAndDump();
               main = std::make_shared<Measurement>(::std::forward<String>(n));
               return main;
            }

            // Can't start twice                                        
            if (found->running and found->name == n)
               return {};

            // Start a new measurement as a child to the currently      
            // running one                                              
            return found->children.emplace_back(
               ::std::make_shared<Measurement>(::std::forward<String>(n))
            );
         }
         else {
            Logger::Error("Profiler: Shouldn't ever be reached: ", LANGULUS_LOCATION());
            return {};
         }
      }
      else {
         // First measurement is always the master measurement          
         // Place it in your main function                              
         main = ::std::make_shared<Measurement>(::std::forward<String>(n));
         return main;
      }
   }

   /// Dump the results into a text file                                      
   void State::DumpProfilerResults() const {
      LANGULUS_PROFILE();
      std::ofstream out;
      out.open(output_file, ::std::ios::out);
      if (not out)
         return;

      for (auto& r : results)
         DumpInner(out, 0, *r.second, 0, Time::min());

      out.close();
   }

   /// Compile all gathered measurements into the results, and write          
   /// statistics to a file, if interval has passed                           
   void State::CompileAndDump() {
      if (Clock::now() > last_output_timestamp + output_interval) {
         // Time to dump the results up until now                       
         last_output_timestamp = Clock::now();
         DumpProfilerResults();
      }

      if (main) {
         // Consume the main measurement and all of its children        
         auto& newChild = results[main->name];
         if (not newChild) {
            newChild = ::std::make_unique<Result>();
            newChild->Integrate(true, *main);
         }
         else newChild->Integrate(false, *main);
         
         main.reset();
      }
   }

   ///                                                                        
   void State::DumpInner(::std::ofstream& out, int depth, const Result& r, int parentSamples, Time parentTime) const {
      // Write name                                                     
      String prefix;
      for (int i = 0; i < depth; ++i)
         prefix += "|  ";
      out << prefix << r.name << ::std::endl;

      // Write how often the function gets called in its parent         
      if (parentSamples and r.samples != parentSamples) {
         if (parentSamples < r.samples) {
            // The execution happens multiple times per parent call     
            out << prefix << "|- happens about " << (r.samples / parentSamples)
                << " times per parent call (on average across " << r.samples << " samples)" << ::std::endl;
         }
         else {
            // The execution happens less often than the parent call    
            int howOften = int((float(r.samples) / float(parentSamples)) * 100.0f);
            out << prefix << "|- has " << howOften
                << "% chance to be called from parent (on average across " << r.samples << ")" << ::std::endl;
         }
      }
      else {
         // The execution happens exactly as much as parent call        
         out << prefix << "|- happens on each parent call (" << r.samples << " samples)" << ::std::endl;
      }

      // Write time stats                                               
      if (r.samples > 1) {
         out << prefix << "|- min time per call: " << RealMs(r.min) << " ms; " << ::std::endl;
         out << prefix << "|- max time per call: " << RealMs(r.max) << " ms; " << ::std::endl;
         out << prefix << "|- avg time per call: " << RealMs(r.average) << " ms; " << ::std::endl;
      }
      else {
         out << prefix << "|- time per call: " << RealMs(r.average) << " ms; " << ::std::endl;
      }

      if (parentSamples and parentSamples < r.samples) {
         const auto total = (double(r.samples) / double(parentSamples)) * RealMs(r.average);
         out << prefix << "|- for total time of: " << total << " ms" << ::std::endl;
      }

      // Write time usage portion                                       
      if (parentTime != Time::min()) {
         auto portion = RealMs(r.average) / RealMs(parentTime);
         out << prefix << "|- consumes " << int(portion * 100.0f)
             << "% of the parent function (average) time" << ::std::endl;
      }

      // Do the same for sub-measurements                               
      if (not r.children.empty()) {
         out << prefix << "|- of which...:" << ::std::endl;
         for (auto& child : r.children) {
            // Make sure results appear in the appropriate depth        
            int depthOffset = 1;
            const Result* parent = &r;
            while (parent and child.second->average > parent->average) {
               parent = parent->parent;
               --depthOffset;
            }

            if (parent)
               DumpInner(out, depth + depthOffset, *child.second, parent->samples, parent->average);
            else
               DumpInner(out, 0, *child.second, 0, Time::min());
         }
      }

      out << ::std::endl;
   }


   State::Measurement::Measurement(String&& n) noexcept
      : name  {::std::forward<String>(n)}
      , start {Clock::now()}
      , end   {start} {}

   void State::Measurement::Stop() noexcept {
      end = Clock::now();
      running = false;
   }

   State::Stopper::Stopper(const MeasurementPtr& m) noexcept
      : measurement {m} {}

   State::Stopper::~Stopper() {
      if (measurement)
         measurement->Stop();
   }

   /// Compile all gathered measurements into a Result                        
   ///   @param isNew - did the measurement already exist in this context?    
   ///   @param m - the measurement to compile                                
   void State::Result::Integrate(bool isNew, const Measurement& m) {
      if (m.running) {
         Logger::Error("Can't integrate measurements while they're still running: ", m.name);
         return;
      }

      const auto duration = m.end - m.start;
      if (isNew) {
         name = m.name;
         min = max = average = duration;
         samples = 1;

         for (auto& sm : m.children) {
            auto& newChild = children[sm->name];
            newChild = ::std::make_unique<Result>();
            newChild->parent = this;
            newChild->Integrate(true, *sm);
         }
      }
      else {
         ++samples;
         average = (((samples - 1) * average) + duration) / samples;

         if (duration < min)
            min = duration;
         if (duration > max)
            max = duration;

         for (auto& sm : m.children) {
            auto& newChild = children[sm->name];
            if (not newChild) {
               newChild = ::std::make_unique<Result>();
               newChild->parent = this;
               newChild->Integrate(true, *sm);
            }
            else newChild->Integrate(false, *sm);
         }
      }
   }

} // namespace Langulus::Profiler
