<?php

/**
 * Base class for WordPress platform config files (civi-config.php).
 *
 */
class Provision_Config_civiplatform extends Provision_Config {
  public $template = 'civi-config.tpl.php';
  public $description = 'civiplatform configuration file';

  function filename() {
    // @todo FIXME was wp-config.php - not sure if relevant for Standalone
    $filename = $this->context->root . '/civi-config.php';
    drush_log(dt("Wordpress: Provision_Config_civiplatform filename = !file", array('!file' => $filename)), 'ok');
    return $filename;
  }

  function write() {
    parent::write();
  }

  function unlink() {
    parent::unlink();
  }

  function process() {
    parent::process();
    $this->data['extra_config'] = "# Hello world from Provision_Config_civiplatform\n";
  }
}
