/*
 * Modified version by Roger VK3KYY - converts bin to SGL file by prepending the header
 * and encrypting the binary
 * 
 * Original version...
 * GD-77 firmware decrypter/encrypter by DG4KLU.
 *
 * Copyright (C)2019 Kai Ludwig, DG4KLU
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
using System.IO;
using System.Collections.Generic;

namespace bin2sgl
{
    class Program
	{
		public enum OutputType
		{
			OutputType_GD77,
			OutputType_DM1801
		}

		private static OutputType outputType = OutputType.OutputType_GD77;

		static void encrypt(string[] args)
        {
			int shift = 0;
			int len = 0;
			int flength = 0;
            FileStream stream_fw_in;
            string outputFilename = Path.GetFileNameWithoutExtension(args[0]) + ".sgl";

			switch(outputType)
			{
				case OutputType.OutputType_GD77:
					len = DataArrays.Header318_0x0807.Length;
					shift = 0x0807;
					flength = 0x77001; // The header, from firmware version 3.1.8 expects the file to be 0x77001 long
					Console.WriteLine("GD77");
					break;

				case OutputType.OutputType_DM1801:
					len = DataArrays.Header219_0x0807.Length;
					shift = 0x2C7C;
					flength = 0x78001; // The header, from firmware version 2.1.9 expects the file to be 0x78001 long
					Console.WriteLine("DM-1801");
					break;
			}

			try
            {
                try
                {
                    stream_fw_in = new FileStream(args[0], FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                }
                catch(Exception)
                {
                    Console.Write("Error: Unable to open file " + args[0]);
                    return;
                }
                FileStream stream_fw_out = new FileStream(outputFilename, FileMode.Create, FileAccess.Write, FileShare.ReadWrite);

				//int len = DataArrays.Header318_0x0807.Length;
				for (int i = 0; i < len; i++)
                {
                    stream_fw_out.WriteByte(((outputType == OutputType.OutputType_GD77) ? DataArrays.Header318_0x0807[i] : DataArrays.Header219_0x0807[i]));
                }

                int length = 0;
                // The header, from firmware version 3.1.8 expects the file to be 0x77001 long
                while (length < flength)
                {
                    int data = stream_fw_in.ReadByte();// This may attempt to read past of the end of the file if its shorter than 0x77001

                    // if readByte was beyond the end of the file, we pad with 0xff
                    if (data < 0)
                    {
                        data = 0xFF;
                    }
                    length++;

                    data = (byte)data ^ DataArrays.EncryptionTable[shift++];
                    data = ~(((data >> 3) & 0b00011111) | ((data << 5) & 0b11100000));

                    stream_fw_out.WriteByte((byte)data);

                    if (shift >= 0x7fff)
                    {
                        shift = 0;
                    }
                }

                stream_fw_in.Close();
                stream_fw_out.Close();
                Console.WriteLine("Created " + outputFilename);
            }
            catch(Exception ex)
            {
                Console.Write("Error :-(\n" + ex.ToString());
            }
		}

		private static string[] RemoveArgAt(string[] args, int index)
		{
			var foos = new List<string>(args);
			foos.RemoveAt(index);
			return foos.ToArray();
		}

		static void Main(string[] args)
		{
			int idx = Array.IndexOf(args, "DM-1801");

			outputType = ((idx >= 0) ? OutputType.OutputType_DM1801 : OutputType.OutputType_GD77);

			if (idx >= 0)
			{
				args = RemoveArgAt(args, idx);
			}

			if (args.Length > 0)
            {
                encrypt(args);
            }
            else
            {
                Console.WriteLine("Usage: bin2sgl [DM-1801] filename");
            }
        }
    }
}