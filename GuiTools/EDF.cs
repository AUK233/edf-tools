using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace GuiTools
{
    internal class EDF
    {
        public static int ChangeModelSize(string FileName, string ScaleSize)
        {
            if (!float.TryParse(ScaleSize, out float newSize))
            {
                newSize = 1;
            }
            return CheckModelXMLHeader(FileName, newSize);
        }

        public static int ChangeCANMVersion(string inPath)
        {
            string outPath;
            if(inPath.Length < 4)
            {
                outPath = inPath + ".xml";
            }
            else
            {
                outPath = inPath.Substring(0, inPath.Length - 4);
                outPath += "_out.xml";
            }
            return CheckCanmXMLVersion(inPath, outPath);
        }


        [DllImport("GuiCore.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        private static extern int CheckModelXMLHeader(string FileName, float ScaleSize);

        [DllImport("GuiCore.dll", CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Ansi)]
        private static extern int CheckCanmXMLVersion(string inPath, string outPath);
    }
}
