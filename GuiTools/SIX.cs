using System;
using System.IO;
using System.Text;

namespace GuiTools
{
	internal class SIX
	{
		public void GenerateMissionTemplate(int i_start, int i_count, string outputPath)
		{
			StringBuilder sb = new StringBuilder();
            sb.AppendLine("\n// Base Mission From \"MISSION\\SAMPLE\\BERSERKERBOMMER\"");
            sb.AppendLine("// internal_EventFactorRegister@ e = internal_CreateEvent(\"__0001_game_event\", \"終了\", 100.00, EVENT_TYPE_SYNC, EVENT_CHECK_AND, int_value);");
            sb.AppendLine("// int_value is this event has N event factors.\n");

            int i_end = i_start + i_count - 1;
            // first part
            for (int i = i_start; i <= i_end; i++)
			{
                string ihex = i.ToString("X4").ToUpper();
                sb.AppendLine($"__EventData __{ihex}_data;");
            }
            sb.AppendLine("\n//---------------------------------------------\n");

            // second part
            for (int i = i_start; i <= i_end; i++)
            {
                string ihex = i.ToString("X4").ToUpper();
                string inum = i.ToString();
                sb.AppendLine($"\tinternal_GetEventState({inum}).SetName(\"noman\"); // __{ihex}_game_event");
            }
            sb.AppendLine("\n//---------------------------------------------");

            sb.AppendLine("// If this event is will be disabled, then this is required.\n");
            // third part
            for (int i = i_start; i <= i_end; i++)
            {
                string ihex = i.ToString("X4").ToUpper();
                string inum = i.ToString();
                sb.AppendLine($"\t//If use: SetBoolState( {inum},::__{ihex}_data.m_is_break,true);");
                sb.AppendLine($"\t//internal_SyncStateEnable({inum}); // __{ihex}_game_event\n");
            }
            sb.AppendLine("\n//---------------------------------------------\n");

            // last part
            for (int i = i_start; i <= i_end; i++)
            {
                string ihex = i.ToString("X4").ToUpper();
                string inum = i.ToString();

                sb.AppendLine("//======================================");
                sb.AppendLine($"// Func_{inum}");
                sb.AppendLine($"void __{ihex}_game_event() {{");
                sb.AppendLine($"\tif( IsBoolState({inum},::__{ihex}_data.m_is_break,::__{ihex}_data.m_is_break) ) return;");
                sb.AppendLine();
                sb.AppendLine($"\t::__{ihex}_data.m_counter++;");
                sb.AppendLine($"\tif( ::__{ihex}_data.m_counter != 1 ) return;");
                sb.AppendLine($"\tMutex_LockObject@ __internal_thread_lock = ::__{ihex}_data.m_mutex.Lock();");
                sb.AppendLine($"\tinternal_InitEventThread({inum}, ::__{ihex}_data.m_counter, \"__{ihex}_game_event\", \"noman\");");
                sb.AppendLine();
                sb.AppendLine($"\tWaitVoice(); // async does not need it.");
                sb.AppendLine($"\tg_soldiers = GetTalkers();");
                sb.AppendLine($"\t::__{ihex}_data.m_game_thread_sync.Reset();");
                sb.AppendLine($"\t::__{ihex}_data.m_voice_thread_sync.Reset();");
                sb.AppendLine();
                sb.AppendLine($"\t__{ihex}_game_event_usercode();");
                sb.AppendLine("}\n");

                sb.AppendLine($"void __{ihex}_game_event_usercode() {{");
                sb.AppendLine($"\tif( IsBoolState({inum},::__{ihex}_data.m_is_pass,::__{ihex}_data.m_is_pass) == false){{");
                sb.AppendLine($"\t\t// do something in here");
                sb.AppendLine($"\t}}");
                sb.AppendLine($"\t// -----------------");
                sb.AppendLine($"\t// next mission is here");
                sb.AppendLine("}\n");
            }

            // write to file
            File.WriteAllText(outputPath, sb.ToString(), Encoding.UTF8);
		}
        // end of class
    }
}
