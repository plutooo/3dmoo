using System;
using System.Collections.Generic;
using System.Text;
using System.Drawing;
using System.IO;

namespace _3dspicconverter
{
    class Program
    {
        static void Main(string[] args)
        {
            // Create a Bitmap object from an image file.
            Bitmap myBitmap = new Bitmap(args[0]);

            // Get the color of a pixel within myBitmap.
            Color pixelColor;

            FileStream fs = new FileStream(args[1],FileMode.OpenOrCreate,FileAccess.Write);

            for (int i = 0; i < myBitmap.Width; i++)
            {
                for (int j = 1; j <= myBitmap.Height; j++)
                {
                    pixelColor = myBitmap.GetPixel(i, myBitmap.Height - j);
                    fs.WriteByte(pixelColor.R);
                    fs.WriteByte(pixelColor.G);
                    fs.WriteByte(pixelColor.B);
                }
            }

        }
    }
}
