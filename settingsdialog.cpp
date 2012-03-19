#include "settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    m_settings()
{
    m_pageCacheSizeLabel = new QLabel(tr("Page cache size:"));
    m_pageCacheSizeLineEdit = new QLineEdit();
    m_pageCacheSizeValidator = new QIntValidator();

    m_pageCacheSizeLineEdit->setValidator(m_pageCacheSizeValidator);

    m_gridLayout = new QGridLayout();
    m_gridLayout->addWidget(m_pageCacheSizeLabel, 0, 0);
    m_gridLayout->addWidget(m_pageCacheSizeLineEdit, 0, 1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_boxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    m_boxLayout->addLayout(m_gridLayout);
    m_boxLayout->addWidget(m_buttonBox);

    this->setLayout(m_boxLayout);

    m_pageCacheSizeLineEdit->setText(m_settings.value("pageObject/pageCacheSize", 134217728).toString());
}

void SettingsDialog::accept()
{
    m_settings.setValue("pageObject/pageCacheSize", m_pageCacheSizeLineEdit->text().toUInt());

    QDialog::accept();
}
