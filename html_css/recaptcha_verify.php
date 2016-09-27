<?php
require_once "recaptchalib.php";

function recaptcha_verify( $g_recaptcha_response )
{
	// your secret key
	$secret = "6LfoGh0TAAAAAK-LgzzTvj1A1ofg0ZKr9p_AMlSk";

	// empty response
	$response = null;

	// check secret key
	$reCaptcha = new ReCaptcha($secret);
	
	// if submitted check response
	if ($g_recaptcha_response)
	{
		$response = $reCaptcha->verifyResponse($_SERVER["REMOTE_ADDR"], $g_recaptcha_response);
	}
	
	return $response != null && $response->success;
}
?>