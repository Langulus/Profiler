///                                                                           
/// Langulus::Profiler                                                        
/// Copyright (c) 2025 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: MIT                                              
///                                                                           
#include <Langulus/Profiler.hpp>
#include <Langulus/Core/Assume.hpp>
#include <fmt/chrono.h>

#if not LANGULUS_FEATURE(PROFILING)
   #error This file shouldn't be built at all if LANGULUS_FEATURE_PROFILING is disabled
#endif


namespace Langulus::Profiler
{

   State Instance {};


   /// Configure the profiler                                                 
   ///   @param profiling_file - file to write results into                   
   ///   @param interval - use zero to disable runtime writing to file        
   void State::Configure(String&& profiling_file, Time interval) noexcept {
      output_file = ::std::forward<String>(profiling_file);
      output_interval = interval;
      last_output_timestamp = Clock::now();
   }

   /// Begin a scoped measurement                                             
   ///   @param n - the name of the measurement, usually the function name    
   ///   @param b - the build configuration (should be inline-generated)      
   ///   @return the auto-stopper                                             
   auto State::Start(String&& n, Build&& b) -> Stopper {
      auto stack = main;
      if (not stack) {
         // First measurement is always the master measurement          
         // Place it in your main function                              
         main = new Measurement (
            ::std::forward<String>(n),
            ::std::forward<Build>(b),
            nullptr
         );
         return main;
      }

      // Otherwise add the new measurement as a child to the previous   
      while (stack->child) {
         // Avoid nesting calls - only the top level is measured        
         if (stack->child->name == n and stack->child->build == b)
            return {};

         stack = stack->child;
      }

      LANGULUS_ASSUME(DevAssumes, not stack->child,
         "A measurement already has children"
      );
      stack->child = new Measurement {
         ::std::forward<String>(n),
         ::std::forward<Build>(b),
         stack
      };
      return stack->child;
   }

   /// End all measurements, compile the results, and write file              
   void State::End() {
      DumpProfilerResults();
   }

   /// Dump the results into a text file                                      
   ///   @param b - the build configuration (should be inline-generated)      
   void State::DumpProfilerResults() const {
      LANGULUS(PROFILE);
      const auto now = ::std::chrono::system_clock::to_time_t(::std::chrono::system_clock::now());
      const auto timestamp = fmt::format("{:%F %T %Z}", fmt::localtime(now));

      std::ofstream out;
      out.open(output_file, ::std::ios::out | ::std::ios::trunc);
      if (not out.is_open())
         Logger::Error("Can't open profiling file: ", output_file);

      out << "<!DOCTYPE html><html>\n";
      out << "<body style = \"color: LightGray; background-color: black; font-family: monospace; font-size: 14px; white-space: pre; \">\n";
      out << "<head><style>\n";
      out << "   div {\n";
      out << "      margin: 0em;\n";
      out << "      padding-left: 2em;\n";
      out << "      line-height: 9px;\n";
      out << "   }\n";
      out << "   h2 {\n";
      out << "      margin: 0em;\n";
      out << "      padding-left: 0em;\n";
      out << "      line-height: 14px;\n";
      out << "   }\n";
      out << "   h3 {\n";
      out << "      margin: 0em;\n";
      out << "      padding-left: 1em;\n";
      out << "      line-height: 12px;\n";
      out << "   }\n";
      out << "   details {\n";
      out << "      margin: 0em;\n";
      out << "      padding-left: 2em;\n";
      out << "      line-height: 12px;\n";
      out << "   }\n";
      out << "</style></head>\n";
      out << "<h2>Last performance results: " << timestamp << "</h2>\n";

      for (auto& r : results)
         for (auto& r2 : r.second)
            r2.second->Dump(out, nullptr);

      out << "</body></html>";
      out.close();
   }

   /// Compile a measurement into the results                                 
   ///   @param b - the measurement to compile                                
   void State::Compile(Measurement* b) {
      LANGULUS_ASSUME(DevAssumes, not b->child,
         "A measurement (", b->name, ") still has a child running (", b->child->name,"), "
         "they should be compiled first when they go out of scope! "
         "Are they on different threads maybe?"
      );

      if (not b->parent) {
         // We're compiling the main measurement                        
         auto& found_function = results[b->name];
         auto& found_build = found_function[b->build];
         if (found_build)
            found_build->Integrate(*b);
         else
            found_build = ::std::make_unique<Result>(*b);

         // Once it stops we dump the results in a file                 
         active_builds.insert(b->build);
         Instance.End();
         return;
      }

      if (b->parent->compiled) {
         // A result already exists, just integrate over it             
         auto& found_function = b->parent->compiled->children[b->name];
         auto& found_build = found_function[b->build];
         if (found_build)
            found_build->Integrate(*b);
         else
            found_build = ::std::make_unique<Result>(*b);

         if (b->ended) {
            // A child has been compiled                                
            active_builds.insert(b->build);
            b->parent->child = nullptr;
         }
         else b->compiled = found_build.get();

         // We still have to climb and update total time for running    
         // results                                                     
         auto p = b->parent;
         while (p and not p->ended) {
            p->compiled->Integrate(*p);
            p = p->parent;
         }
      }
      else {
         // We have to build the result hierarchy from the ground up    
         // and cache it so we don't have to do it again                
         auto node = main;
         while (node) {
            if (not node->compiled) {
               auto& found_function = node->parent
                  ? node->parent->compiled->children[node->name]
                  : results[node->name];
               auto& found_build = found_function[node->build];
               if (found_build)
                  found_build->Integrate(*node);
               else
                  found_build = ::std::make_unique<Result>(*node);

               if (node->parent and node->ended) {
                  // A measurement has been compiled                    
                  active_builds.insert(node->build);
                  node->parent->child = nullptr;
                  break;
               }
               
               node->compiled = found_build.get();
            }

            node = node->child;
         }
      }

      if (output_interval != 0s and Clock::now() > last_output_timestamp + output_interval) {
         // Time to dump the results up until now                       
         last_output_timestamp = Clock::now();
         DumpProfilerResults();
      }
   }

   State::Measurement::Measurement(String&& n, Build&& b, Measurement* p) noexcept
      : name   {::std::forward<String>(n)}
      , build  {::std::forward<Build>(b)}
      , start  {Clock::now()}
      , end    {start}
      , parent {p} {
      LANGULUS_ASSUME(DevAssumes, not parent or not parent->child,
         "A parent already has a child"
      );
   }

   void State::Measurement::Stop() noexcept {
      end = Clock::now();
      ended = true;
      Instance.Compile(this);
   }

   /// Compile a measurement into a Result                                    
   ///   @param m - the measurement to compile                                
   State::Result::Result(const Measurement& m) {
      name = m.name;
      build = m.build;

      if (m.ended) {
         const auto duration = m.end - m.start;
         min = max = average = total = duration;
         samples = 1;
      }
      else total = Clock::now() - m.start;
   }

   /// Compile a measurement into an already existing Result                  
   ///   @param m - the measurement to compile                                
   void State::Result::Integrate(const Measurement& m) {
      if (not m.ended) {
         if (samples == 0)
            total = Clock::now() - m.start;
         return;
      }

      const auto duration = m.end - m.start;
      if (samples == 0) {
         // First measurement                                           
         min = max = average = total = duration;
         samples = 1;
      }
      else {
         // Consecutive measurements (averaging a sample)               
         ++samples;
         average = (((samples - 1) * average) + duration) / samples;
         total += duration;

         if (duration < min)
            min = duration;
         if (duration > max)
            max = duration;
      }
   }
   
   /// Write a result as HTML                                                 
   ///   @param out - file to write to                                        
   ///   @param parent - parent result for contextualizing data               
   void State::Result::Dump(::std::ofstream& out, const Result* parent) const {
      // Write name and build                                           
      const Real hot = parent ? RealMs(total) / RealMs(parent->total) : 1_real;
      const auto hex = Logger::Hex(build);
      const bool act = Instance.active_builds.contains(build) and hot > 0.25_real;

      // Color-code hot results:                                        
      //    -> blue if relative_hotness goes to zero                    
      //    -> white if relative_hotness goes to 0.5                    
      //    -> red if relative_hotness goes to 1                        
      int red = 255;
      int green = 255;
      int blue = 255;
      if (parent) {
         const Real relative_hotness = std::max(std::min(hot, 1_real), 0_real);
         if (relative_hotness < 0.5f)
            red = green = 128 + static_cast<int>((relative_hotness * 2_real) * 128_real);
         else
            blue = green = 255 - static_cast<int>((relative_hotness * 2_real - 1_real) * 128_real);
      }

      // Write the measurement heading                                  
      if (act) {
         out << "<details open style=\"color:rgb("<<red<<","<<green<<","<<blue<<");\"><summary><h3>" << name
             << " [BUILD: " << std::string(std::begin(hex), std::end(hex)) << "]</h3></summary>\n";
      }
      else {
         out << "<details      style=\"color:rgb("<<red<<","<<green<<","<<blue<<");\"><summary><h3>" << name
             << " [BUILD: " << std::string(std::begin(hex), std::end(hex)) << "]</h3></summary>\n";
      }

      // Write how often the function gets called in its parent         
      if (parent and parent->samples) {
         if (samples != parent->samples) {
            if (parent->samples < samples) {
               // The execution happens multiple times per parent call  
               out << "<div>- happens about " << (samples / parent->samples)
                   << " times per parent call</div>\n";
            }
            else {
               // The execution happens less often than the parent call 
               int howOften = int((float(samples) / float(parent->samples)) * 100.0f);
               out << "<div>- has " << howOften
                   << "% chance to be called from parent</div>\n";
            }
         }
         else {
            // The execution happens exactly as much as parent call     
            out << "<div>- happens on each parent call</div>\n";
         }
      }

      // Write time stats                                               
      if (samples > 1) {
         out << "<div>- min time per call: " << RealMs(min)     << " ms;</div>\n";
         out << "<div>- avg time per call: " << RealMs(average) << " ms;</div>\n";
         out << "<div>- max time per call: " << RealMs(max)     << " ms;</div>\n";
         out << "<div>- " << samples << " executions, for total time: " << RealMs(total) << " ms;</div>\n";
      }
      else if (samples == 1) {
         out << "<div>- 1 execution, for total time: " << RealMs(total) << " ms;</div>\n";
      }
      else {
         out << "<div>- <span style=\"background-color: ForestGreen;\">still running...</span> total time until now: " << RealMs(total) << " ms;</div>\n";
      }

      // Write time usage portion                                       
      if (parent) {
         auto portion = RealMs(total) / RealMs(parent->total);
         out << "<div>- consumes " << int(portion * 100.0f)
               << "% of the parent function total time </div>\n";
      }

      // Do the same for sub-measurements                               
      if (not children.empty()) {
         out << "<div>of which:</div>\n";
         for (auto& cccc : children)
            for (auto& child : cccc.second)
               child.second->Dump(out, this);
      }

      out << "</details>\n";
   }

} // namespace Langulus::Profiler
