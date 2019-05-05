using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GD77_FirmwareLoader
{
	static class Program
	{
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main(string[] args)
		{
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
				if (args.Length > 1 && args[1]=="GUI")
				{
					FrmProgress frmProgress = new FrmProgress();
					frmProgress.SetLabel("");
					frmProgress.SetProgressPercentage(0);
					frmProgress.Show();
					FirmwareLoader.UploadFirmare(args[0], frmProgress);
					frmProgress.Close();
				}
				else
				{
					FirmwareLoader.UploadFirmare(args[0]);
				}
			}
			//	Console.WriteLine("Usage GD77_FirmwareLoader filename");
		}
	}
}
