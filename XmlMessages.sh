function get_files
{
    echo tellico.xml
}

function po_for_file
{
    case "$1" in
       tellico.xml)
           echo tellico_xml_mimetypes.po
       ;;
    esac
}

function tags_for_file
{
    case "$1" in
       tellico.xml)
           echo comment
       ;;
    esac
}
