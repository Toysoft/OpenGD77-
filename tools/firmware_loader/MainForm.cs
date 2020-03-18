/*
 * Copyright (C)2019 Roger Clark. VK3KYY
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.ComponentModel.Design;
using System.IO;
using System.Net;
using System.Text.RegularExpressions;
#if (LINUX_BUILD)
#else
using UsbLibrary;
#endif

namespace GD77_FirmwareLoader
{
	public partial class MainForm : Form
	{

		private static String tempFile = "";
		private WebClient wc = null;

		public MainForm()
		{
			InitializeComponent();
		}

		private void rbModel_CheckedChanged(object sender, EventArgs e)
		{
			RadioButton rb = sender as RadioButton;

			if (rb != null)
			{
				if (rb.Checked)
				{
					FirmwareLoader.outputType = (FirmwareLoader.OutputType)rb.Tag;
				}
			}
		}

		private void enableUI(bool state)
		{
			this.grpboxModel.Enabled = state;
			this.btnDownload.Enabled = state;
			this.btnOpenFile.Enabled = state;
		}

		private void downloadProgressChangedCallback(object sender, DownloadProgressChangedEventArgs ev)
		{
			this.progressBar.Value = ev.ProgressPercentage;
		}

		private void downloadStringCompletedCallback(object sender, DownloadStringCompletedEventArgs ev)
		{
			String result = ev.Result;
			String urlBase = "http://github.com";
			String pattern = "";
			String urlFW = "";

			this.progressBar.Visible = false;

			// Define Regex's patterm, according to current Model selection
			switch (FirmwareLoader.outputType)
			{
				case FirmwareLoader.OutputType.OutputType_GD77:
					pattern = @"/rogerclarkmelbourne/OpenGD77/releases/download/R([0-9\.]+)/OpenGD77\.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_GD77S:
					pattern = @"/rogerclarkmelbourne/OpenGD77/releases/download/R([0-9\.]+)/OpenGD77S_HS\.sgl";
					break;
				case FirmwareLoader.OutputType.OutputType_DM1801:
					pattern = @"/rogerclarkmelbourne/OpenGD77/releases/download/R([0-9\.]+)/OpenDM1801\.sgl";
					break;
			}

			// Looking for firmware's URL
			String[] lines = result.Split('\n');
			foreach (String l in lines)
			{
				Match match = Regex.Match(l, pattern, RegexOptions.IgnoreCase);

				if (match.Success)
				{
					urlFW = match.Groups[0].Value;
					break;
				}
			}

			// Is firmware's URL found ?
			if (urlFW.Length > 0)
			{
				tempFile = System.IO.Path.GetTempPath() + Guid.NewGuid().ToString() + ".sgl";

				// Download the firmware binary to a temporary file
				try
				{
					Application.DoEvents();
					this.progressBar.Value = 0;
					this.progressBar.Visible = true;
					wc.DownloadFileAsync(new Uri(urlBase + urlFW), tempFile);
				}
				catch (Exception ex)
				{
					MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);

					if (File.Exists(tempFile))
						File.Delete(tempFile);

					enableUI(true);
					this.progressBar.Visible = false;
					return;
				}
			}
		}

		private void downloadFileCompletedCallback(object sender, AsyncCompletedEventArgs ev)
		{
			this.progressBar.Visible = false;
			this.progressBar.Value = 0;

			// Now flash the downloaded firmware
			try
			{
				FrmProgress frmProgress = new FrmProgress();
				frmProgress.SetLabel("");
				frmProgress.SetProgressPercentage(0);
				frmProgress.FormBorderStyle = FormBorderStyle.FixedSingle;
				frmProgress.MaximizeBox = false;
				frmProgress.Show();

				if (FirmwareLoader.UploadFirmware(tempFile, frmProgress) != 0)
				{
					MessageBox.Show("Error: Unable to upload the firmware to the " + FirmwareLoader.getModelName(), "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				}

				frmProgress.Close();
			}
			catch (Exception ex)
			{
				MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
			}

			// Cleanup
			if (File.Exists(tempFile))
				File.Delete(tempFile);

			enableUI(true);
		}

		private void btnDownload_Click(object sender, EventArgs e)
		{
			Uri uri = new Uri("https://github.com/rogerclarkmelbourne/OpenGD77/releases/latest");

			wc = new WebClient();
			ServicePointManager.SecurityProtocol = SecurityProtocolType.Ssl3 | SecurityProtocolType.Tls | SecurityProtocolType.Tls11 | SecurityProtocolType.Tls12;

			this.progressBar.Value = 0;

			wc.DownloadProgressChanged += new DownloadProgressChangedEventHandler(downloadProgressChangedCallback);
			wc.DownloadStringCompleted += new DownloadStringCompletedEventHandler(downloadStringCompletedCallback);
			wc.DownloadFileCompleted += new AsyncCompletedEventHandler(downloadFileCompletedCallback);

			this.progressBar.Visible = true;
			enableUI(false);

			// Retrieve release webpage
			try
			{
				Application.DoEvents();
				wc.DownloadStringAsync(uri);
			}
			catch (Exception ex)
			{
				MessageBox.Show("Error: " + ex.Message, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
				enableUI(true);
				this.progressBar.Visible = false;
				return;
			}
		}

		private void btnOpenFile_Click(object sender, EventArgs e)
		{
			OpenFileDialog openFileDialog1 = new OpenFileDialog();
			openFileDialog1.Filter = "firmware files (*.sgl)|*.sgl|binary files (*.bin)|*.bin|All files (*.*)|*.*";
			openFileDialog1.RestoreDirectory = true;

			if (openFileDialog1.ShowDialog() == DialogResult.OK)
			{
				try
				{
					enableUI(false);
					FrmProgress frmProgress = new FrmProgress();
					frmProgress.SetLabel("");
					frmProgress.SetProgressPercentage(0);
					frmProgress.FormBorderStyle = FormBorderStyle.FixedSingle;
					frmProgress.MaximizeBox = false;
					frmProgress.Show();
					FirmwareLoader.UploadFirmware(openFileDialog1.FileName, frmProgress);
					frmProgress.Close();
				}
				catch (Exception)
				{

				}
				enableUI(true);
			}
		}
	}
}
