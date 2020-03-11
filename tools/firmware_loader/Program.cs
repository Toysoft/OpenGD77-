using System;
using System.Linq;
using System.Collections.Generic;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GD77_FirmwareLoader
{
	static class Program
	{
		private static string[] RemoveArgAt(string[] args, int index)
		{
			var foos = new List<string>(args);
			foos.RemoveAt(index);
			return foos.ToArray();
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
			int exitCode = 0;
			/* Testing only
			   args = new string[2];
			   args[0] = "test.bin";
			   args[1] = "GUI";
			*/
			if (args.Length == 0)
			{
				Application.EnableVisualStyles();
				Application.SetCompatibleTextRenderingDefault(false);
				Application.Run(new MainForm());
			}
			else
			{
				int idxDM1801 = Array.IndexOf(args, "DM-1801");
				int idxGD77S = Array.IndexOf(args, "GD-77S");


				if (idxDM1801 >= 0)
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_DM1801;
					args = RemoveArgAt(args, idxDM1801);
				}
				else if(idxGD77S >= 0)
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77S;
					args = RemoveArgAt(args, idxGD77S);
				}
				else
				{
					FirmwareLoader.outputType = FirmwareLoader.OutputType.OutputType_GD77;
				}

				if (args.Contains("--help") || args.Contains("-h") || args.Contains("/h"))
				{
					Console.WriteLine("\nUsage: GD77_FirmwareLoader [GUI] [DM-1801 | GD-77S] [filename]\n\n");
					Environment.Exit(exitCode);
				}

				if (args.Length == 0)
				{
					Application.EnableVisualStyles();
					Application.SetCompatibleTextRenderingDefault(false);
					Application.Run(new MainForm());
				}

				int idx = Array.IndexOf(args, "GUI");
				if (idx >= 0)
				{
					args = RemoveArgAt(args, idx);

					if (args.Length <= 0)
					{
						Console.WriteLine("ERROR: No filename specified.");
						Environment.Exit(-1);
					}

					FrmProgress frmProgress = new FrmProgress();
					frmProgress.SetLabel("");
					frmProgress.SetProgressPercentage(0);
					frmProgress.Show();

					exitCode = FirmwareLoader.UploadFirmware(args[0], frmProgress);
					frmProgress.Close();
				}
				else
				{
					if (args.Length <= 0)
					{
						Console.WriteLine("ERROR: No filename specified.");
						Environment.Exit(-1);
					}

					exitCode = FirmwareLoader.UploadFirmware(args[0]);
				}
			}
			//	Console.WriteLine("Usage GD77_FirmwareLoader filename");

			Environment.Exit(exitCode);
		}
	}
}
