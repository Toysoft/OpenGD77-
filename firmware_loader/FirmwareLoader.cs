//#define EXTENDED_DEBUG
/*
 * 
 * Copyright (C)2019 Roger Clark. VK3KYY
 * 
 * Encryption sections based on work by Kai DG4KLU
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
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UsbLibrary;
using System.IO;
using System.Windows.Forms;


namespace GD77_FirmwareLoader
{
	class FirmwareLoader
	{
		private static readonly byte[] responseOK = { 0x41 };

		private static SpecifiedDevice _specifiedDevice = null;
		private static FrmProgress _progessForm;

		public static int UploadFirmare(string fileName,FrmProgress progessForm=null)
		{
			_progessForm = progessForm;
			_specifiedDevice = SpecifiedDevice.FindSpecifiedDevice(0x15A2, 0x0073);
			if (_specifiedDevice == null)
			{
				Console.WriteLine("Error. Can't connect to the GD-77");
				return -1;
			}

			byte[] fileBuf = encrypt(File.ReadAllBytes(fileName));
			if (fileBuf.Length > 0x7b000)
			{
				Console.WriteLine("\nError. Firmware file too large.");
				return -2;
			}

			if (sendInitialCommands() == true)
			{
				int respCode = sendFileData(fileBuf);
				if (respCode == 0)
				{
					Console.WriteLine("\nFirmware update complete. Please reboot the GD-77");
				}
				else
				{
					switch (respCode)
					{
						case -1:
							Console.WriteLine("\nError. File to large");
							break;
						case -2:
						case -3:
						case -4:
						case -5:
							Console.WriteLine("\nError " + respCode + " While sending data file");
							break;
					}
					return -3;
				}
			}
			else
			{
				Console.WriteLine("\nError while sending initial commands");
				return -4;
			}
			return 0;
		}

		static bool sendAndCheckResponse(byte[] cmd, byte[] resp)
		{
			const int TRANSFER_LENGTH = 38;
			byte[] responsePadded = new byte[TRANSFER_LENGTH];
			byte[] recBuf = new byte[TRANSFER_LENGTH];

			if (resp.Length < TRANSFER_LENGTH)
			{
				Buffer.BlockCopy(resp, 0, responsePadded, 0, resp.Length);
			}

			_specifiedDevice.SendData(cmd);
			_specifiedDevice.ReceiveData(recBuf);// Wait for response

			if (recBuf.SequenceEqual(responsePadded))
			{
				return true;
			}
			else
			{
				Console.WriteLine("Error read returned ");
				return false;
			}
		}

		static private byte[] createChecksumData(byte[] buf, int startAddress, int endAddress)
		{
			//checksum data starts with a small header, followed by the 32 bit checksum value, least significant byte first
			byte[] checkSumData = { 0x45, 0x4e, 0x44, 0xff, 0xDE, 0xAD, 0xBE, 0xEF };
			int cs = 0;
			for (int i = startAddress; i < endAddress; i++)
			{
				cs = cs + (int)buf[i];
			}

			checkSumData[4] = (byte)(cs % 256);
			checkSumData[5] = (byte)((cs >> 8) % 256);
			checkSumData[6] = (byte)((cs >> 16) % 256);
			checkSumData[7] = (byte)((cs >> 24) % 256);

			return checkSumData;
		}

		static private void updateBlockAddressAndLength(byte[] buf, int address, int length)
		{
			// Length is 16 bits long in bytes 5 and 6
			buf[5] = (byte)((length) % 256);
			buf[4] = (byte)((length >> 8) % 256);

			// Address is 4 bytes long, in the first 4 bytes
			buf[3] = (byte)((address) % 256);
			buf[2] = (byte)((address >> 8) % 256);
			buf[1] = (byte)((address >> 16) % 256);
			buf[0] = (byte)((address >> 24) % 256);
		}

		static private int sendFileData(byte[] fileBuf)
		{
			byte[] dataHeader = new byte[0x20 + 0x06];
			const int BLOCK_LENGTH = 1024;
			int dataTransferSize = 0x20;
			int checksumStartAddress = 0;
			int address = 0;

			if (_progessForm != null)
			{
				_progessForm.SetLabel("Programming data");
			}



			int fileLength = fileBuf.Length;
			int totalBlocks = (fileLength / BLOCK_LENGTH) + 1;


			while (address < fileLength)
			{

				if (address % BLOCK_LENGTH == 0)
				{
					checksumStartAddress = address;
				}

				updateBlockAddressAndLength(dataHeader, address, dataTransferSize);


				if (address + dataTransferSize < fileLength)
				{
					Buffer.BlockCopy(fileBuf, address, dataHeader, 6, 32);

					if (sendAndCheckResponse(dataHeader, responseOK) == false)
					{
						Console.WriteLine("Error sending data");
						return -2;
					}

					address = address + dataTransferSize;
					if ((address % 0x400) == 0)
					{
						if (_progessForm != null)
						{
							_progessForm.SetProgressPercentage((address * 100 / BLOCK_LENGTH) / totalBlocks);
						}
#if EXTENDED_DEBUG
						Console.WriteLine("Sent block " + (address / BLOCK_LENGTH) + " of " + totalBlocks);
#else
						Console.Write(".");
#endif
						if (sendAndCheckResponse(createChecksumData(fileBuf, checksumStartAddress, address), responseOK) == false)
						{
							Console.WriteLine("Error sending checksum");
							return -3;
						}
					}
				}
				else
				{
#if EXTENDED_DEBUG
					Console.WriteLine("Sending last block");
#else
					Console.Write(".");
#endif

					dataTransferSize = fileLength - address;
					updateBlockAddressAndLength(dataHeader, address, dataTransferSize);
					Buffer.BlockCopy(fileBuf, address, dataHeader, 6, dataTransferSize);

					if (sendAndCheckResponse(dataHeader, responseOK) == false)
					{
						Console.WriteLine("Error sending data");
						return -4;
					}

					address = address + dataTransferSize;

					if (sendAndCheckResponse(createChecksumData(fileBuf, checksumStartAddress, address), responseOK) == false)
					{
						Console.WriteLine("Error sending checksum");
						return -5;
					}
				}
			}
			return 0;
		}

		static private bool sendInitialCommands()
		{
			byte[] commandLetterA = new byte[] { 0x41 }; //A
			byte[][] command0 = new byte[][] { new byte[] { 0x44, 0x4f, 0x57, 0x4e, 0x4c, 0x4f, 0x41, 0x44 }, new byte[] { 0x23, 0x55, 0x50, 0x44, 0x41, 0x54, 0x45, 0x3f } }; // DOWNLOAD
			byte[][] command1 = new byte[][] { commandLetterA, responseOK };
			byte[][] command2 = new byte[][] { new byte[] { 0x44, 0x56, 0x30, 0x31, (0x61 + 0x00), (0x61 + 0x0C), (0x61 + 0x0D), (0x61 + 0x01) }, new byte[] { 0x44, 0x56, 0x30, 0x31 } }; //.... last 4 bytes of the command are the offset encoded as letters a - p (hard coded fr
			byte[][] command3 = new byte[][] { new byte[] { 0x46, 0x2d, 0x50, 0x52, 0x4f, 0x47, 0xff, 0xff }, responseOK }; //... F-PROG..
			byte[][] command4 = new byte[][] { new byte[] { 0x53, 0x47, 0x2d, 0x4d, 0x44, 0x2d, 0x37, 0x36, 0x30, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }, responseOK }; //SG-MD-760
			byte[][] command5 = new byte[][] { new byte[] { 0x4d, 0x44, 0x2d, 0x37, 0x36, 0x30, 0xff, 0xff }, responseOK }; //MD-760..
			byte[][] command6 = new byte[][] { new byte[] { 0x56, 0x31, 0x2e, 0x30, 0x30, 0x2e, 0x30, 0x31 }, responseOK }; //V1.00.01
			byte[][] commandErase = new byte[][] { new byte[] { 0x46, 0x2d, 0x45, 0x52, 0x41, 0x53, 0x45, 0xff }, responseOK }; //F-ERASE
			byte[][] commandPostErase = new byte[][] { commandLetterA, responseOK };
			byte[][] commandProgram = { new byte[] { 0x50, 0x52, 0x4f, 0x47, 0x52, 0x41, 0x4d, 0xf }, responseOK };//PROGRAM
			byte[][][] commands = { command0, command1, command2, command3, command4, command5, command6, commandErase, commandPostErase, commandProgram };
			int commandNumber = 0;

			// Send the commands which the GD-77 expects before the start of the data
			while (commandNumber < commands.Length)
			{
				if (_progessForm != null)
				{
					_progessForm.SetLabel("Sending command " + commandNumber);
				}
#if EXTENDED_DEBUG
				Console.WriteLine("Sending command " + commandNumber);
#else
				Console.Write(".");
#endif

				if (sendAndCheckResponse(commands[commandNumber][0], commands[commandNumber][1]) == false)
				{
					Console.WriteLine("Error sending command ");
					return false;
				}
				commandNumber = commandNumber + 1;
			}
			return true;
		}

		static byte[] encrypt(byte [] unencrypted)
        {
            int shift = 0x0807;
			byte [] encrypted = new byte[unencrypted.Length];
			int data;

				byte[] encryptionTable  = new byte[32768];
				int len = unencrypted.Length;
				for(int address=0;address<len;address++)
				{
					data = unencrypted[address] ^ DataTable.EncryptionTable[shift++];

					data = ~(((data >> 3) & 0x1F) | ((data << 5) & 0xE0)); // 0x1F is 0b00011111   0xE0 is 0b11100000

					encrypted[address] = (byte)data;

					if (shift >= 0x7fff)
					{
						shift = 0;
					}
                }
				return encrypted;
        }
	}
}
