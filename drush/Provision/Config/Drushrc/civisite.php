<?php
/**
 * @file
 * Provides the Provision_Config_Drushrc_civisite class.
 */

/**
 * Class for writing $platform/sites/$url/drushrc.php files. (FIXME)
 */
class Provision_Config_Drushrc_civisite extends Provision_Config_Drushrc {
  protected $context_name = 'civisite';
  public $template = 'provision_drushrc_civisite.tpl.php';
  public $description = 'civisite Drush configuration file';

  function filename() {
    return $this->site_path . '/drushrc.php';
  }
}
