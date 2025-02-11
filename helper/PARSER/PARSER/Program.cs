using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.Diagnostics.Tracing.Parsers;
using Microsoft.Diagnostics.Tracing.Etlx;
using Microsoft.Diagnostics.Tracing;
using System.IO;

namespace ConsoleApp2
{
    internal class Program
    {
        static void Main(string[] args)
        {
            if (args.Length < 2)
            {
                Console.WriteLine("please give etl file and target file path.");
                return;
            }
    
            string sourceFileName = args[0];
            string targetFileName = args[1];
            long delta = 504911232000000000L + 288000000000L;

            // check if sourceFile exists
            if (!File.Exists(sourceFileName))
            {
                Console.WriteLine($"etl file {sourceFileName} doesn't exist!");
                return;
            }

            try
            {
                using (StreamWriter writer = new StreamWriter(targetFileName))
                {
                    var source = new ETWTraceEventSource(sourceFileName);
                    writer.WriteLine(source.SessionStartTime.Ticks - delta);
                    // Console.WriteLine(source.SessionStartTime.Ticks - delta);
                    // var parser = new DynamicTraceEventParser(source); 

                    //var etlxFile = TraceLog.CreateFromEventTraceLogFile(sourceFileName);
                    //var traceLog = new TraceLog(etlxFile);
                    //Console.WriteLine(traceLog.SessionStartTime.Ticks - delta);
                    // writer.WriteLine(traceLog.SessionStartTime.Ticks - delta);

                }
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.ToString());
            }
        }
    }
}
