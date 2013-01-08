#ifndef PRINTOPTIONS_H
#define PRINTOPTIONS_H

struct PrintOptions
{
    bool fitToPage;
    bool landscape;

    QString pageRanges;

    enum PageSet
    {
        AllPages = 0,
        EvenPages = 1,
        OddPages = 2
    };

    PageSet pageSet;

    enum NumberUp
    {
        SinglePage = 0,
        TwoPages = 1,
        FourPages = 2,
        SixPages = 3,
        NinePages = 4,
        SixteenPages = 5
    };

    NumberUp numberUp;

    enum NumberUpLayout
    {
        BottomTopLeftRight = 0,
        BottomTopRightLeft = 1,
        LeftRightBottomTop = 2,
        LeftRightTopBottom = 3,
        RightLeftBottomTop = 4,
        RightLeftTopBottom = 5,
        TopBottomLeftRight = 6,
        TopBottomRightLeft = 7
    };

    NumberUpLayout numberUpLayout;

    PrintOptions() : fitToPage(false), landscape(false), pageRanges(), pageSet(AllPages), numberUp(SinglePage), numberUpLayout(LeftRightTopBottom) {}

};

#endif // PRINTOPTIONS_H
