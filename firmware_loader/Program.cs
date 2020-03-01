using System;
using System.Collections.Generic;
using System.Linq;
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
				int idx = Array.IndexOf(args, "DM-1801");

				FirmwareLoader.outputType = ((idx >= 0) ? FirmwareLoader.OutputType.OutputType_DM1801 : FirmwareLoader.OutputType.OutputType_GD77);

				if (idx >= 0)
				{
					args = RemoveArgAt(args, idx);
				}

				if (args.Contains("--help") || args.Contains("-h") || args.Contains("/h"))
				{
					Console.WriteLine("\nUsage: GD77_FirmwareLoader [GUI] [DM-1801] [filename]\n\n");
					Environment.Exit(exitCode);
				}

				if (args.Length == 0)
				{
					Application.EnableVisualStyles();
					Application.SetCompatibleTextRenderingDefault(false);
					Application.Run(new MainForm());
				}

				idx = Array.IndexOf(args, "GUI");
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

					bool curs = Console.CursorVisible;
					Console.CursorVisible = false;
					exitCode = FirmwareLoader.UploadFirmware(args[0]);
					Console.CursorVisible = curs;
				}
			}
			//	Console.WriteLine("Usage GD77_FirmwareLoader filename");

			Environment.Exit(exitCode);
		}
	}
}
