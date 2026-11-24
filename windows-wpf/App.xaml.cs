using System;
using System.Threading;
using System.Windows;

namespace DesktopShredder
{
    public partial class App : Application
    {
        private Mutex? _singleInstanceMutex;

        protected override void OnStartup(StartupEventArgs e)
        {
            _singleInstanceMutex = new Mutex(true, @"Local\DesktopShredder_SingleInstance_Mutex", out bool createdNew);
            if (!createdNew)
            {
                // Prevent duplicate floating widgets if user launches executable again
                Shutdown(0);
                return;
            }

            base.OnStartup(e);

            MainWindow mainWindow = new MainWindow();
            MainWindow = mainWindow;
            mainWindow.Show();
        }

        protected override void OnExit(ExitEventArgs e)
        {
            try
            {
                _singleInstanceMutex?.ReleaseMutex();
                _singleInstanceMutex?.Dispose();
            }
            catch
            {
            }

            base.OnExit(e);
        }
    }
}
