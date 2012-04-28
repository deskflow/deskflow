<?php

namespace Synergy;

class Files {

  function downloadRequested() {
    return ($_GET["page"] == "help") && stristr($_SERVER["REQUEST_URI"], "user-guide.pdf");
  }
  
  function download() {
    $file = "help/user-guide.pdf";
    if (file_exists($file)) {
      header('Content-Description: File Transfer');
      header('Content-Type: application/octet-stream');
      header('Content-Disposition: attachment; filename='.basename($file));
      header('Content-Transfer-Encoding: binary');
      header('Expires: 0');
      header('Cache-Control: must-revalidate');
      header('Pragma: public');
      header('Content-Length: ' . filesize($file));
      ob_clean();
      flush();
      readfile($file);
    }
    else {
      header("HTTP/1.0 404 Not Found");
      print("User guide missing.");
    }
  }
}

?>