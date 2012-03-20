#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QtCore>
#include <QtGui>

#include "pageobject.h"

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent = 0);

public slots:
    void accept();
    
private:
    QBoxLayout *m_boxLayout;
    QGridLayout *m_gridLayout;

    QDialogButtonBox *m_buttonBox;

    QLabel *m_pageCacheThreadingLabel;
    QCheckBox *m_pageCacheThreadingCheckBox;

    QLabel *m_pageCacheSizeLabel;
    QLineEdit *m_pageCacheSizeLineEdit;
    QIntValidator *m_pageCacheSizeValidator;

    QSettings m_settings;

};

#endif // SETTINGSDIALOG_H
