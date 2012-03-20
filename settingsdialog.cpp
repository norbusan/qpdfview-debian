#include "settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) : QDialog(parent),
    m_settings()
{
    m_pageCacheThreadingLabel = new QLabel(tr("Page cache threading:"));
    m_pageCacheThreadingCheckBox = new QCheckBox();

    m_pageCacheSizeLabel = new QLabel(tr("Page cache size:"));
    m_pageCacheSizeLineEdit = new QLineEdit();
    m_pageCacheSizeValidator = new QIntValidator();

    m_pageCacheSizeLineEdit->setValidator(m_pageCacheSizeValidator);

    m_gridLayout = new QGridLayout();
    m_gridLayout->addWidget(m_pageCacheThreadingLabel, 0, 0);
    m_gridLayout->addWidget(m_pageCacheThreadingCheckBox, 0, 1);
    m_gridLayout->addWidget(m_pageCacheSizeLabel, 1, 0);
    m_gridLayout->addWidget(m_pageCacheSizeLineEdit, 1, 1);

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    m_boxLayout = new QBoxLayout(QBoxLayout::TopToBottom);
    m_boxLayout->addLayout(m_gridLayout);
    m_boxLayout->addWidget(m_buttonBox);

    this->setLayout(m_boxLayout);

    m_pageCacheThreadingCheckBox->setChecked(m_settings.value("pageObject/pageCacheThreading", true).toBool());
    m_pageCacheSizeLineEdit->setText(m_settings.value("pageObject/pageCacheSize", 134217728).toString());
}

void SettingsDialog::accept()
{
    m_settings.setValue("pageObject/pageCacheThreading", m_pageCacheThreadingCheckBox->isChecked());
    PageObject::setPageCacheThreading(m_pageCacheThreadingCheckBox->isChecked());

    m_settings.setValue("pageObject/pageCacheSize", m_pageCacheSizeLineEdit->text().toUInt());
    PageObject::setPageCacheSize(m_pageCacheSizeLineEdit->text().toUInt());

    QDialog::accept();
}
