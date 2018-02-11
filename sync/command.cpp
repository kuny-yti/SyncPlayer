#include "command.h"

command *command::_this_ = NULL;
command::command (const QString &pf, QSettings::Format fm):
    ini(pf, fm)
{
    _this_ = this;

    QStringList keys = ini.allKeys();
    for (int i = 0; i < keys.size(); ++i)
        key_value[keys[i]] = ini.value(keys[i]);
}
command::~command()
{
    QStringList keys = key_value.keys();
    for (int i = 0; i < keys.size(); ++i)
        ini.setValue(keys[i], key_value[keys[i]]);
    ini.sync();
}

//! 指定命令的value
QVariant &command::operator [] (eCommandLists pc)
{
    return key_value[CommadKeyList[pc].keykey];
}
QVariant command::operator [] (eCommandLists pc)const
{
    return key_value[CommadKeyList[pc].keykey];
}
QString command::key(eCommandLists pc, bool is_net)const
{
    if (is_net)
        return CommadKeyList[pc].netkey;
    return CommadKeyList[pc].keykey;
}
QStringList command::keys(eCommandLists pc, bool is_net)const
{
    QStringList multkey;
    QString nkey ;
    QStringList allkey = key_value.keys();

    if (is_net)
        nkey = CommadKeyList[pc].netkey;
    else
        nkey = CommadKeyList[pc].keykey;

    if (nkey.contains(KeySplit))
    {
        QStringList mnkey = nkey.split(KeySplit);
        nkey = mnkey[0];
    }
    else
        return QStringList();
    for (int i =0 ; i < allkey.count(); ++i)
        if (allkey[i].contains(nkey))
            multkey.append(allkey[i]);

    return multkey;
}
//! 指定key的value
QVariant &command::operator [] (const QString &key)
{
    return key_value[key];
}
QVariant command::operator [] (const QString &key)const
{
    return key_value[key];
}
void command::update()
{
    QStringList keys = key_value.keys();
    for (int i = 0; i < keys.size(); ++i)
        ini.setValue(keys[i], key_value[keys[i]]);
    ini.sync();
}

#include <QKeyEvent>
#include <QMouseEvent>

QList<int> command::event(eCommandLists cl, eKeyModes &mode)
{
    QList<int> retke;
    command_key keycmd = command_key(operator [](cl).toString());

    if (keycmd.is_valid())
    {
        QStringList kclist = keycmd.keys();
        mode = keycmd.mode;
        for (int i = 0; i < kclist.count(); ++i)
            retke.append(keycmd[kclist[i]]);
    }

    return retke;
}

command_key::command_key(const QString &val):
    key(),
    value(val),
    mode(Key_NotMode)
{
    // Key_L | Key_K
    if (val.contains(KeyOrSplit))
    {
        mode = Key_OrMode;
        QStringList orlist = val.split(KeyOrSplit);
        for (int i = 0; i < orlist.size(); ++i)
            key[orlist[i]] = spilt_value(orlist[i]);
    }
    // Key_L & Key_K
    else if (val.contains(KeyAndSplit))
    {
        mode = Key_AndMode;
        QStringList orlist = val.split(KeyAndSplit);
        for (int i = 0; i < orlist.size(); ++i)
            key[orlist[i]] = spilt_value(orlist[i]);
    }
    // Key_L + Key_K
    else if (val.contains(KeyModifierSplit))
    {
        mode = Key_ModifierMode;
        QStringList orlist = val.split(KeyModifierSplit);
        for (int i = 0; i < orlist.size(); ++i)
            key[orlist[i]] = spilt_value(orlist[i]);
    }
    // Key_L
    else
    {
        mode = Key_SingleMode;
        key[val] = spilt_value(val);
    }
}

bool command_key::is_valid()const{return key.count() && !value.isEmpty() && mode != Key_NotMode;}
QStringList command_key::keys()const { return key.keys();}
int command_key::operator [](const QString &k) {return key[k];}
int command_key::operator [](const QString &k)const{return key[k];}

int command_key::spilt_value(const QString &keyval)
{
    // Key_L
    QString trikey = keyval.trimmed();
    if (trikey.contains(KeySplit))
    {
        QStringList klist = trikey.split(KeySplit);
        if (klist.size() == 2 && klist[0] == KeyBoardKey)
        {
            QString val = klist[1];
            val = val.trimmed();
            if (val.count() == 1)
            {
                QChar keychar = *val.data();
                if (keychar.isDigit() || keychar.isSymbol() || keychar.isLetter())
                    return keychar.toLatin1();

            }
            else if (val.count() > 1)
            {
                QString upper = val.trimmed().toUpper();
                int list_count = sizeof(KeyBoardCodeList)/sizeof(KeyBoardCodeList[0]);
                for (int i =0 ; i < list_count; ++i)
                {
                    if (upper == KeyBoardCodeList[i].keyval)
                        return KeyBoardCodeList[i].keycode;
                }
            }
        }
    }
    return -1;
}
