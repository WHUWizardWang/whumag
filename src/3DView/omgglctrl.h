#ifndef OMGGLCTRL_H
#define OMGGLCTRL_H

#include <QWidget>

namespace Ui
{
    class OmgGLCtrl;
}

class OmgGLCtrl : public QWidget
{
    Q_OBJECT

public:
    explicit OmgGLCtrl(QWidget *parent = nullptr);
    ~OmgGLCtrl();
    int scale();

signals:
    void verticalSliderXMoved(int value);
    void verticalSliderYMoved(int value);
    void verticalSliderZMoved(int value);
    void verticalSliderScaleMoved(int value);

    void pushButtonRawViewClicked();
    void pushButtonMoveViewClicked(bool isChecked);
    void pushButtonZoomInClicked();
    void pushButtonZoomOutClicked();

    void colorMapChanged(const QString &colorMapName);
    void colorMapInversed(bool isInverse);
    void shadeClosed(bool isCloseShade);
    void textureClosed(bool isCloseTexture);

    void magColorMapChanged(const QString &colorMapName);
    void magColorMapInversed(bool isInverse);
    void displayModeChanged(int mode);

public slots:
    void moveSliderX(int value);
    void moveSliderY(int value);
    void moveSliderZ(int value);
    void changeCheckboxTexture(bool hasTexture);

private slots:
    void on_verticalSlider_x_sliderMoved(int position);

    void on_verticalSlider_y_sliderMoved(int position);

    void on_verticalSlider_z_sliderMoved(int position);

    void on_pushButton_rawView_clicked();

    void on_pushButton_zoomIn_clicked();

    void on_pushButton_zoomOut_clicked();

    void on_pushButton_moveView_clicked(bool checked);

    void on_verticalSlider_scale_sliderMoved(int position);

    void on_comboBox_color_currentTextChanged(const QString &arg1);

    void on_checkBox_inverse_toggled(bool checked);

    void on_checkBox_closeShade_toggled(bool checked);

    void on_checkBox_closeTexture_toggled(bool checked);

    void on_comboBox_color_2_currentIndexChanged(const QString &arg1);

    void on_checkBox_inverse_2_toggled(bool checked);

    void on_radioButton_1_toggled(bool checked);

    void on_radioButton_2_toggled(bool checked);

    void on_radioButton_3_toggled(bool checked);

private:
    Ui::OmgGLCtrl *ui;
};

#endif // OMGGLCTRL_H
