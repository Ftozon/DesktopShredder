using System;
using System.Windows;
using System.Windows.Controls;

namespace DesktopShredder
{
    public partial class SettingsWindow : Window
    {
        private readonly AppConfig _config;
        private readonly Action _onApplied;

        public SettingsWindow(AppConfig config, Action onApplied)
        {
            InitializeComponent();
            _config = config;
            _onApplied = onApplied;

            AutoStartCheckBox.IsChecked = StartupManager.IsAutoStartEnabled();
            LockPositionCheckBox.IsChecked = _config.LockPosition;
            OpacitySlider.Value = _config.WidgetOpacity;
            OpacityValueText.Text = $"{(int)Math.Round(_config.WidgetOpacity * 100)}%";

            SelectPassesItem(_config.ShredPasses);
        }

        private void SelectPassesItem(int passes)
        {
            string targetTag = passes >= 35 ? "35" : (passes <= 3 ? "3" : "7");
            foreach (object item in PassCountComboBox.Items)
            {
                if (item is ComboBoxItem cbi && string.Equals(cbi.Tag?.ToString(), targetTag, StringComparison.Ordinal))
                {
                    PassCountComboBox.SelectedItem = cbi;
                    return;
                }
            }
            PassCountComboBox.SelectedIndex = 0;
        }

        private void OpacitySlider_ValueChanged(object sender, RoutedPropertyChangedEventArgs<double> e)
        {
            if (OpacityValueText != null)
            {
                OpacityValueText.Text = $"{(int)Math.Round(e.NewValue * 100)}%";
            }
        }

        private void Save_Click(object sender, RoutedEventArgs e)
        {
            bool wantAutoStart = AutoStartCheckBox.IsChecked == true;
            StartupManager.SetAutoStart(wantAutoStart);

            _config.LockPosition = LockPositionCheckBox.IsChecked == true;
            _config.WidgetOpacity = Math.Clamp(OpacitySlider.Value, 0.40, 1.0);

            if (PassCountComboBox.SelectedItem is ComboBoxItem selected &&
                int.TryParse(selected.Tag?.ToString(), out int parsedPasses))
            {
                _config.ShredPasses = parsedPasses;
            }

            _config.Save();
            _onApplied();
            Close();
        }

        private void Cancel_Click(object sender, RoutedEventArgs e)
        {
            Close();
        }
    }
}
