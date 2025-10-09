using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using static System.Net.Mime.MediaTypeNames;

namespace GuiTools
{
	public partial class MainWindow : Window
	{
		public MainWindow()
		{
			InitializeComponent();
		}

		private void TextBox_DragOver(object sender, DragEventArgs e)
		{
			e.Effects = DragDropEffects.Copy;
			e.Handled = true;
		}

		private void TextBox_Drop(object sender, DragEventArgs e)
		{
			if (e.Data.GetDataPresent(DataFormats.FileDrop))
			{
				if (sender is TextBox tb)
				{
					tb.Text = "";
					string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);
					if (files != null && files.Length > 0)
					{
						tb.Text = files[0];
					}
				}
			}
		}

		private void GotoModelScale_Click(object sender, RoutedEventArgs e)
		{
			string path = ModelPathBox.Text;
			if (path != "")
			{
				Button button = sender as Button;
				button.Content = "Doing...";
				int errorCode = EDF.ChangeModelSize(path, ModelScaleBox.Text);
				if(errorCode == 0)
				{
					button.Content = "Next";
					System.Media.SystemSounds.Beep.Play();
				}
				else
				{
					button.Content = "Done";
				}
			}
			else
			{
				System.Media.SystemSounds.Hand.Play();
			}
		}

		private void GotoCANMResolver_Click(object sender, RoutedEventArgs e)
		{
			string path = CANMPathBox.Text;
			if (path != "")
			{
				Button button = sender as Button;
				button.Content = "Doing...";
				int errorCode = EDF.ChangeCANMVersion(path);
				if (errorCode == 0)
				{
					button.Content = "Next";
					System.Media.SystemSounds.Beep.Play();
				}
				else
				{
					button.Content = "Done";
				}
			}
			else
			{
				System.Media.SystemSounds.Hand.Play();
			}
		}

		private void GotoEDF6Mission_Click(object sender, RoutedEventArgs e)
		{
            // You should change this path!
            string path = "Z:\\TEMP\\MissionTemplate.txt";

			Button button = sender as Button;
			button.Content = "Doing";

			SIX edf6mission = new SIX();
			edf6mission.GenerateMissionTemplate(
				int.Parse(EDF6Mission1.Text),
				int.Parse(EDF6Mission2.Text),
				path);

			button.Content = "Next";
			System.Media.SystemSounds.Beep.Play();
		}
	}
}
