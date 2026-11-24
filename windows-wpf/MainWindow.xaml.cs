using System;
using System.Diagnostics;
using System.Threading;
using System.Threading.Channels;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace DesktopShredder
{
    public partial class MainWindow : Window
    {
        private readonly AppConfig _config;
        private readonly uint _currentPid;
        private readonly Channel<string[]> _shredQueue;
        private readonly DispatcherTimer _fullscreenTimer;
        private readonly DispatcherTimer _animTimer;

        private int _activeJobs;
        private double _animPhase;
        private bool _hiddenByFullscreen;
        private SettingsWindow? _settingsWindow;

        private static readonly SolidColorBrush LedIdleBrush = CreateFrozenBrush(0x22, 0xC5, 0x5E);     // Emerald Ready
        private static readonly SolidColorBrush LedHoverBrush = CreateFrozenBrush(0xF5, 0x9E, 0x0B);    // Amber Drop Ready
        private static readonly SolidColorBrush LedShredBrush = CreateFrozenBrush(0xEF, 0x44, 0x44);    // Crimson Shredding
        private static readonly SolidColorBrush HeadNormalBrush = CreateFrozenBrush(0x47, 0x55, 0x69);
        private static readonly SolidColorBrush HeadHoverBrush = CreateFrozenBrush(0x33, 0x41, 0x55);
        private static readonly SolidColorBrush ThroatNormalBrush = CreateFrozenBrush(0x0F, 0x17, 0x2A);
        private static readonly SolidColorBrush ThroatHoverBrush = CreateFrozenBrush(0xDC, 0x26, 0x26);

        public MainWindow()
        {
            InitializeComponent();

            _config = AppConfig.Load();
            _currentPid = (uint)Environment.ProcessId;
            _shredQueue = Channel.CreateUnbounded<string[]>(new UnboundedChannelOptions
            {
                SingleReader = true,
                SingleWriter = false
            });

            ApplyConfigToWindow(setInitialPosition: true);

            // Background consumer loop for silent, non-blocking multi-pass shredding
            _ = Task.Run(ProcessShredQueueAsync);

            // Poll every 400ms to hide widget automatically when any other app enters fullscreen
            _fullscreenTimer = new DispatcherTimer(DispatcherPriority.Background)
            {
                Interval = TimeSpan.FromMilliseconds(400)
            };
            _fullscreenTimer.Tick += FullscreenTimer_Tick;
            _fullscreenTimer.Start();

            // Smooth 30 FPS silent visual animation while actively shredding
            _animTimer = new DispatcherTimer(DispatcherPriority.Render)
            {
                Interval = TimeSpan.FromMilliseconds(33)
            };
            _animTimer.Tick += AnimTimer_Tick;
        }

        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);

            // Set WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE so the floating shredder stays off Alt+Tab
            // and never steals focus from the user's active window.
            IntPtr hwnd = new WindowInteropHelper(this).Handle;
            if (hwnd != IntPtr.Zero)
            {
                long exStyle = NativeMethods.GetWindowLong(hwnd, NativeMethods.GWL_EXSTYLE);
                exStyle |= NativeMethods.WS_EX_TOOLWINDOW | NativeMethods.WS_EX_NOACTIVATE;
                NativeMethods.SetWindowLong(hwnd, NativeMethods.GWL_EXSTYLE, exStyle);
            }
        }

        public void ApplyConfigToWindow(bool setInitialPosition)
        {
            RootContainer.Opacity = _config.WidgetOpacity;
            RootContainer.Cursor = _config.LockPosition ? Cursors.Arrow : Cursors.SizeAll;

            if (setInitialPosition)
            {
                Rect workArea = SystemParameters.WorkArea;
                if (_config.WindowLeft.HasValue && _config.WindowTop.HasValue)
                {
                    Left = Math.Clamp(_config.WindowLeft.Value, SystemParameters.VirtualScreenLeft,
                        SystemParameters.VirtualScreenLeft + SystemParameters.VirtualScreenWidth - Width);
                    Top = Math.Clamp(_config.WindowTop.Value, SystemParameters.VirtualScreenTop,
                        SystemParameters.VirtualScreenTop + SystemParameters.VirtualScreenHeight - Height);
                }
                else
                {
                    Left = workArea.Right - Width - 28;
                    Top = workArea.Bottom - Height - 28;
                }
            }
        }

        private async Task ProcessShredQueueAsync()
        {
            await foreach (string[] targets in _shredQueue.Reader.ReadAllAsync())
            {
                Interlocked.Increment(ref _activeJobs);
                _ = Dispatcher.InvokeAsync(UpdateVisualState);

                try
                {
                    int passes = _config.ShredPasses;
                    SecureShredder.ShredTargets(targets, passes);
                }
                finally
                {
                    Interlocked.Decrement(ref _activeJobs);
                    _ = Dispatcher.InvokeAsync(UpdateVisualState);
                }
            }
        }

        private void UpdateVisualState()
        {
            bool busy = Volatile.Read(ref _activeJobs) > 0;
            if (busy)
            {
                StatusLed.Fill = LedShredBrush;
                if (!_animTimer.IsEnabled)
                {
                    _animPhase = 0;
                    _animTimer.Start();
                }
            }
            else
            {
                _animTimer.Stop();
                StatusLed.Fill = LedIdleBrush;
                Canvas.SetTop(PaperSheet, 0);
                ResetStrips();
            }
        }

        private void AnimTimer_Tick(object? sender, EventArgs e)
        {
            _animPhase += 0.18;

            // Animate paper feeding down into cutter slot
            double paperOffset = (_animPhase * 5.0) % 24.0;
            Canvas.SetTop(PaperSheet, paperOffset);

            // Pulse red status LED & animate shredded strips fluttering in window
            StatusLed.Opacity = 0.55 + 0.45 * Math.Abs(Math.Sin(_animPhase * 2.0));

            Rectangle[] strips = { Strip1, Strip2, Strip3, Strip4, Strip5, Strip6 };
            double[] baseHeights = { 22, 29, 18, 31, 24, 27 };
            for (int i = 0; i < strips.Length; i++)
            {
                double wave = 5.0 * Math.Sin(_animPhase * 2.6 + i * 1.15);
                strips[i].Height = Math.Clamp(baseHeights[i] + wave, 10, 33);
            }
        }

        private void ResetStrips()
        {
            StatusLed.Opacity = 1.0;
            Strip1.Height = 22;
            Strip2.Height = 29;
            Strip3.Height = 18;
            Strip4.Height = 31;
            Strip5.Height = 24;
            Strip6.Height = 27;
        }

        private void FullscreenTimer_Tick(object? sender, EventArgs e)
        {
            // Keep visible if Settings dialog is currently open
            if (_settingsWindow != null && _settingsWindow.IsVisible)
            {
                return;
            }

            bool otherFullscreen = NativeMethods.IsAnotherAppFullscreen(_currentPid);
            if (otherFullscreen && !_hiddenByFullscreen)
            {
                _hiddenByFullscreen = true;
                Visibility = Visibility.Collapsed;
            }
            else if (!otherFullscreen && _hiddenByFullscreen)
            {
                _hiddenByFullscreen = false;
                Visibility = Visibility.Visible;
            }
        }

        private void Window_MouseLeftButtonDown(object sender, MouseButtonEventArgs e)
        {
            if (_config.LockPosition)
            {
                return;
            }

            if (e.ButtonState == MouseButtonState.Pressed)
            {
                try
                {
                    DragMove();
                    SaveCurrentPosition();
                }
                catch
                {
                }
            }
        }

        private void Window_MouseLeftButtonUp(object sender, MouseButtonEventArgs e)
        {
            SaveCurrentPosition();
        }

        private void SaveCurrentPosition()
        {
            _config.WindowLeft = Left;
            _config.WindowTop = Top;
            _config.Save();
        }

        private void Window_DragEnter(object sender, DragEventArgs e)
        {
            HandleDragHover(e, entering: true);
        }

        private void Window_DragOver(object sender, DragEventArgs e)
        {
            HandleDragHover(e, entering: true);
        }

        private void Window_DragLeave(object sender, DragEventArgs e)
        {
            SetHoverVisual(false);
        }

        private void HandleDragHover(DragEventArgs e, bool entering)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effects = (e.AllowedEffects & DragDropEffects.Move) != 0
                    ? DragDropEffects.Move
                    : DragDropEffects.Copy;
                if (entering)
                {
                    SetHoverVisual(true);
                }
            }
            else
            {
                e.Effects = DragDropEffects.None;
            }
            e.Handled = true;
        }

        private void SetHoverVisual(bool hover)
        {
            CutterHead.Background = hover ? HeadHoverBrush : HeadNormalBrush;
            SlotThroat.Background = hover ? ThroatHoverBrush : ThroatNormalBrush;
            if (Volatile.Read(ref _activeJobs) == 0)
            {
                StatusLed.Fill = hover ? LedHoverBrush : LedIdleBrush;
            }
        }

        private void Window_Drop(object sender, DragEventArgs e)
        {
            SetHoverVisual(false);

            if (e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                if (e.Data.GetData(DataFormats.FileDrop) is string[] droppedPaths && droppedPaths.Length > 0)
                {
                    // Silent immediate destruction: enqueue directly with zero confirmation popups
                    _shredQueue.Writer.TryWrite(droppedPaths);
                }
            }

            e.Handled = true;
        }

        private void OpenSettings_Click(object sender, RoutedEventArgs e)
        {
            if (_settingsWindow != null && _settingsWindow.IsLoaded)
            {
                _settingsWindow.Activate();
                return;
            }

            _settingsWindow = new SettingsWindow(_config, () => ApplyConfigToWindow(setInitialPosition: false));
            _settingsWindow.Closed += (_, _) => _settingsWindow = null;
            _settingsWindow.Show();
        }

        private void ResetPosition_Click(object sender, RoutedEventArgs e)
        {
            Rect workArea = SystemParameters.WorkArea;
            Left = workArea.Right - Width - 28;
            Top = workArea.Bottom - Height - 28;
            SaveCurrentPosition();
        }

        private void Exit_Click(object sender, RoutedEventArgs e)
        {
            SaveCurrentPosition();
            Application.Current.Shutdown();
        }

        private static SolidColorBrush CreateFrozenBrush(byte r, byte g, byte b)
        {
            SolidColorBrush brush = new SolidColorBrush(Color.FromRgb(r, g, b));
            brush.Freeze();
            return brush;
        }
    }
}
