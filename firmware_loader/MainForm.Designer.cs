using System.ComponentModel.Design;
using System.Windows.Forms;
using System;
namespace GD77_FirmwareLoader
{
	partial class MainForm
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.btnOpenFile = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// btnOpenFile
			// 
			this.btnOpenFile.Location = new System.Drawing.Point(39, 12);
			this.btnOpenFile.Name = "btnOpenFile";
			this.btnOpenFile.Size = new System.Drawing.Size(144, 23);
			this.btnOpenFile.TabIndex = 0;
			this.btnOpenFile.Text = "Select file and upload";
			this.btnOpenFile.UseVisualStyleBackColor = true;
			this.btnOpenFile.Click += new System.EventHandler(this.btnOpenFile_Click);
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(221, 49);
			this.Controls.Add(this.btnOpenFile);
			this.KeyPreview = true;
			this.Name = "MainForm";
			this.Text = "GD-77 Firmware loader";
			this.ResumeLayout(false);

		}

		#endregion

		private Button btnOpenFile;


	}
}

